/* Console helper. Compile this PRG alongside the kit's oads_hb.c.
   Returns text only. It does not connect, change data or print secrets. */
FUNCTION OAds_ServerStatsText( hStats )
LOCAL cText := "", cKey, xValue, cSub, nWidth := 76
IF ! HB_ISHash( hStats )
   RETURN "Server stats unavailable" + hb_eol()
ENDIF
cText += PadR( "Statistic", 24 ) + PadL( "Current", 20 ) + ;
         PadL( "Max used", 16 ) + PadL( "Rejected", 16 ) + hb_eol()
cText += Replicate( "-", nWidth ) + hb_eol()
FOR EACH cKey IN HB_HKeys( hStats )
   xValue := hStats[ cKey ]
   IF HB_ISHash( xValue ) .AND. hb_HHasKey( xValue, "current" )
      cText += UsageLabel( Label( cKey ) ) + PadL( Value( xValue["current"] ), 20 ) + ;
               PadL( Value( StatsField( xValue, "max_used" ) ), 16 ) + PadL( Value( StatsField( xValue, "rejected" ) ), 16 ) + hb_eol()
   ELSEIF HB_ISHash( xValue )
      FOR EACH cSub IN HB_HKeys( xValue )
         cText += Lines( Label( cKey ) + "/" + Label( cSub ), Value( xValue[cSub] ) )
      NEXT
   ELSE
      cText += Lines( Label( cKey ), ;
               iif( cKey == "max_sessions" .AND. HB_ISNumeric(xValue) .AND. xValue == 0, ;
                    "Unlimited (0)", Value(xValue) ) )
   ENDIF
NEXT
RETURN cText

STATIC FUNCTION Label( cKey )
RETURN StrTran( cKey, "_", " " )

STATIC FUNCTION Value( x )
DO CASE
CASE x == NIL
   RETURN "Unavailable"
CASE HB_ISNumeric(x)
   RETURN AllTrim( Str(x, 20, iif(x == Int(x), 0, 2)) )
CASE HB_ISString(x)
   RETURN StrTran( StrTran( x, Chr(13), " " ), Chr(10), " " )
CASE HB_ISLogical(x)
   RETURN iif(x, "True", "False")
ENDCASE
RETURN hb_ValToStr(x)

STATIC FUNCTION Lines( cLabel, cValue )
LOCAL cText := "", lFirst := .T.
/* Long labels get their own line, never collide with the value column. */
IF Len(cLabel) > 24
   DO WHILE Len(cLabel) > 0
      cText += Left(cLabel,76) + hb_eol()
      cLabel := SubStr(cLabel,77)
   ENDDO
ENDIF
IF Len(cValue) == 0
   RETURN PadR(cLabel,24) + hb_eol()
ENDIF
DO WHILE Len(cValue) > 0
   cText += PadR( iif(lFirst,cLabel,""),24 ) + ;
            iif(Len(cValue) <= 20, PadL(cValue,20), Left(cValue,52)) + hb_eol()
   cValue := SubStr(cValue,53)
   lFirst := .F.
ENDDO
RETURN cText

STATIC FUNCTION StatsField( h, cKey )
IF hb_HHasKey( h, cKey )
   RETURN h[cKey]
ENDIF
RETURN NIL

STATIC FUNCTION UsageLabel( cLabel )
LOCAL cText := ""
IF Len(cLabel) > 24
   DO WHILE Len(cLabel) > 0
      cText += Left(cLabel,76) + hb_eol()
      cLabel := SubStr(cLabel,77)
   ENDDO
ENDIF
RETURN cText + PadR(cLabel,24)
