/* Runtime exposure proof only. No clock value is added to the protocol. */
FUNCTION OADS_AUTH_V1(cNonce,cBlob,cContext)
 LOCAL tNow:=hb_DateTime(),tFixed:=hb_DateTime(2026,1,15,12,34,56,789)
 LOCAL cText:=hb_TSToStr(tFixed),cRaw:=hb_TToS(tFixed)
 LOCAL tUTC:=hb_TSToUTC(tFixed),nOffset:=hb_UTCOffset(tFixed)
 HB_SYMBOL_UNUSED(cNonce)
 HB_SYMBOL_UNUSED(cBlob)
 HB_SYMBOL_UNUSED(cContext)
 IF Len(hb_TSToStr(tNow))!=23 .OR. cText!="2026-01-15 12:34:56.789" .OR. cRaw!="20260115123456789"
  RETURN "TIME48-FAIL-FORMAT"
 ENDIF
 IF hb_TSToStr(hb_StrToTS(cText))!=cText .OR. hb_TToS(hb_SToT(cRaw))!=cRaw
  RETURN "TIME48-FAIL-PARSE"
 ENDIF
 IF hb_TSToStr(hb_UTCToTS(tUTC))!=cText
  RETURN "TIME48-FAIL-UTC"
 ENDIF
RETURN "TIME48-PASS|"+cText+"|"+cRaw+"|"+hb_TSToStr(tUTC)+"|"+Str(nOffset,10,0)
