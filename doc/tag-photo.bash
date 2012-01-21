#!/bin/bash
JPG="$1"
LOG="$2"

( type sqlite3 && type xmlstarlet && type exiftool && type gawk && type curl ) &>/dev/null \
|| { echo "couldn't find something useful" ; exit 1 ; }

[[ -z "$WPS_U" ]] && WPS_U="*PUT_ME_HERE*"
[[ -z "$WPS_R" ]] && WPS_R="*PUT_ME_HERE*"
[[ -z "$LCACHE" ]] && LCACHE="/tmp/iii-locations-cache.sqlite"
wps_locate() {
    local APS="$1"
    [[ -z "$APS" ]] && return;
    local APSKEY="$(md5sum <<<"$APS"|gawk -- '{print $1}')"
    local RV=""
    [[ -r "$LCACHE" ]] \
    && RV="$(sqlite3 "$LCACHE" "SELECT v FROM lc WHERE k='$APSKEY'")" \
    || sqlite3 "$LCACHE" 'CREATE TABLE lc ( k varchar PRIMARY KEY, v varchar )'
    [[ -z "$RV" ]] || { echo "$RV" ; return 0 ; }
    X="$(curl -s -H 'Content-Type: text/xml' -d "<?xml version='1.0'?><LocationRQ xmlns='http://skyhookwireless.com/wps/2005' version='2.6' street-address-lookup='none'><authentication version='2.0'><simple><username>$WPS_U</username><realm>$WPS_R</realm></simple></authentication>$APS</LocationRQ>" https://api.skyhookwireless.com/wps2/location)"
    RV="$(xmlstarlet sel -N w=http://skyhookwireless.com/wps/2005 -t -m '/w:LocationRS/w:location/*' -o wl_ -v 'name()' -o '=' -v 'text()' -o ' ' - <<<"$X")"
    sqlite3 "$LCACHE" "INSERT INTO lc (k,v) VALUES ('$APSKEY','$RV')" &>/dev/null
    echo "$RV"
    return 0
}

ts="$(TZ=UTC date +%s -d "$(exiftime -tc "$JPG"|cut -d\  -f3-|sed -e s/:/-/ -e s/:/-/)")"
APS="$(gawk <"$LOG" -F, -v ts="$ts" -- '
BEGIN { nap=0; }
$3=="NEWPHOTO" && $2>ts { pdt=$1; nextfile; }
$3=="AP" || $3=="NEWAP" {
    n = ($4 in ap) ? ap[$4] : nap;
    ap_dt[n] = $1; ap_ap[n]=$4; ap_rssi[n]=$5;
    if(n==nap) ap[$4] = nap++;
}
$3=="POWERON" { nap=0; delete ap_dt; delete ap_ap; delete ap_rssi; delete ap; }
END {
    for(i=0;i<nap;++i) {
	if((pdt-ap_dt[i])>1800) continue;
	printf("<access-point><mac>%s</mac><signal-strength>-%d</signal-strength></access-point>\n",ap_ap[i],ap_rssi[i]);
    }
}
')"

[[ -z "$APS" ]] && { echo "no access points" ; return 2>/dev/null || exit ; }
WL="$(wps_locate "$APS")"
[[ -z "$WL" ]] && { echo "couldn't find location" ; return 2>/dev/null || exit ; }
eval "$WL"
[[ -z "$wl_latitude" || -z "$wl_longitude" || -z "$wl_hpe" ]] && { echo "invalid location ($WL)"; return 2>/dev/null || exit; }
[[ "${wl_latitude:0:1}" = '-' ]] && wl_latitude="${wl_latitude:1}" wl_latitude_ref=S || wl_latitude_ref=N
[[ "${wl_longitude:0:1}" = '-' ]] && wl_longitude="${wl_longitude:1}" wl_longitude_ref=W || wl_longitude_ref=E
exiftool -GPSLatitude="$wl_latitude" -GPSLongitude="$wl_longitude" -GPSLatitudeRef=$wl_latitude_ref -GPSLongitudeRef=$wl_longitude_ref -GPSVersionID=0.0.2.2 -GPSProcessingMethod=WLAN "$JPG"
# XXX: exiftool doesn't like it  -GPSHPositioningError="$wl_hpe"
