<?xml version="1.0" encoding="us-ascii"?>
<xsl:stylesheet version="1.0"
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 >
 <xsl:output
  method="text"
  encoding="us-ascii"
  media-type="text/plain" />

 <xsl:template match="/">
  <xsl:apply-templates select="/Config/Cards/Card"/>
 </xsl:template>

 <xsl:template match="/Config/Cards/Card">
  <xsl:text>cat &gt;</xsl:text>
  <xsl:value-of select="translate(@MacAddress,'-','')"/>
  <xsl:text>.conf &lt;&lt;__EOF__&#xa;</xsl:text>
  <xsl:text>uploadkey=&quot;</xsl:text>
  <xsl:value-of select="UploadKey"/>
  <xsl:text>&quot;&#xa;</xsl:text>
  <xsl:text>targetdir=&quot;/var/lib/iii/%s&quot;&#xa;</xsl:text>
  <xsl:text>umask=022&#xa;</xsl:text>
  <xsl:text>__EOF__&#xa;</xsl:text>
 </xsl:template>

 <xsl:template match="*|text()"/>

</xsl:stylesheet>
