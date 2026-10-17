/* Deliberate hostile fixtures. Never use as a real auth module. */
FUNCTION OADS_AUTH_V1(cNonce,cBlob,cContext)
 LOCAL x
 HB_SYMBOL_UNUSED(cNonce)
 HB_SYMBOL_UNUSED(cContext)
 DO CASE
 CASE cBlob=="huge-string"
  x:=Replicate("X",536870912)
 CASE cBlob=="huge-array"
  x:=Array(100000000)
 CASE cBlob=="spin"
  DO WHILE .T.
  ENDDO
 CASE cBlob=="runtime-error"
  x:=Len(9)
 ENDCASE
 HB_SYMBOL_UNUSED(x)
RETURN "OAUTH"+Chr(1)+Chr(1)+Chr(3)+Chr(0)+"usr"+Chr(2)+Chr(0)+"pw"
