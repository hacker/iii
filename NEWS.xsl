<?xml version="1.0" encoding="us-ascii"?>
<xsl:stylesheet version="1.0"
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 >
 <xsl:output
  method="text"
  encoding="us-ascii"
  media-type="text/plain" />

 <xsl:template match="news">
  <xsl:apply-templates/>
 </xsl:template>
 <xsl:template match="version">
  <xsl:value-of select="concat(@version,' (',@date,')&#xA;')"/>
  <xsl:apply-templates/>
 </xsl:template>
 <xsl:template match="ni">
  <xsl:text>  - </xsl:text>
  <xsl:apply-templates mode="text"/>
  <xsl:text>&#xA;</xsl:text>
 </xsl:template>
 <xsl:template match="*|text()"/>
 
</xsl:stylesheet>
