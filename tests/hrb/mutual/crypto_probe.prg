FUNCTION OADS_AUTH_V1(cNonce,cBlob,cContext)
 LOCAL cA,cB,cKey,cCipher,a,h
 HB_SYMBOL_UNUSED(cNonce)
 HB_SYMBOL_UNUSED(cBlob)
 HB_SYMBOL_UNUSED(cContext)
 IF Lower(hb_sha256(""))!="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
  RETURN "FAIL-HASH-sha256"
 ENDIF
 IF Lower(hb_sha256("abc"))!="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
  RETURN "FAIL-HASH-sha256"
 ENDIF
 IF Len(hb_sha256("abc",.T.))!=32
  RETURN "FAIL-RAW-sha256"
 ENDIF
 IF Lower(hb_HMAC_sha256("Hi There",Replicate(Chr(11),20)))!="b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"
  RETURN "FAIL-HMAC-sha256"
 ENDIF
 IF Len(hb_HMAC_sha256("Hi There",Replicate(Chr(11),20),.T.))!=32
  RETURN "FAIL-HMAC-RAW-sha256"
 ENDIF
 IF Lower(hb_sha384(""))!="38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b"
  RETURN "FAIL-HASH-sha384"
 ENDIF
 IF Lower(hb_sha384("abc"))!="cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7"
  RETURN "FAIL-HASH-sha384"
 ENDIF
 IF Len(hb_sha384("abc",.T.))!=48
  RETURN "FAIL-RAW-sha384"
 ENDIF
 IF Lower(hb_HMAC_sha384("Hi There",Replicate(Chr(11),20)))!="afd03944d84895626b0825f4ab46907f15f9dadbe4101ec682aa034c7cebc59cfaea9ea9076ede7f4af152e8b2fa9cb6"
  RETURN "FAIL-HMAC-sha384"
 ENDIF
 IF Len(hb_HMAC_sha384("Hi There",Replicate(Chr(11),20),.T.))!=48
  RETURN "FAIL-HMAC-RAW-sha384"
 ENDIF
 IF Lower(hb_sha512(""))!="cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"
  RETURN "FAIL-HASH-sha512"
 ENDIF
 IF Lower(hb_sha512("abc"))!="ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"
  RETURN "FAIL-HASH-sha512"
 ENDIF
 IF Len(hb_sha512("abc",.T.))!=64
  RETURN "FAIL-RAW-sha512"
 ENDIF
 IF Lower(hb_HMAC_sha512("Hi There",Replicate(Chr(11),20)))!="87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cdedaa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854"
  RETURN "FAIL-HMAC-sha512"
 ENDIF
 IF Len(hb_HMAC_sha512("Hi There",Replicate(Chr(11),20),.T.))!=64
  RETURN "FAIL-HMAC-RAW-sha512"
 ENDIF
 IF Len(OADS_RandomBytes(1))!=1
  RETURN "FAIL-RANDOM-LENGTH"
 ENDIF
 IF Len(OADS_RandomBytes(32))!=32
  RETURN "FAIL-RANDOM-LENGTH"
 ENDIF
 IF Len(OADS_RandomBytes(1024))!=1024
  RETURN "FAIL-RANDOM-LENGTH"
 ENDIF
 IF OADS_RandomBytes(0)!=NIL
  RETURN "FAIL-RANDOM-BOUND"
 ENDIF
 IF OADS_RandomBytes(-1)!=NIL
  RETURN "FAIL-RANDOM-BOUND"
 ENDIF
 IF OADS_RandomBytes(1025)!=NIL
  RETURN "FAIL-RANDOM-BOUND"
 ENDIF
 IF OADS_RandomBytes(1.5)!=NIL
  RETURN "FAIL-RANDOM-BOUND"
 ENDIF
 IF OADS_RandomBytes("32")!=NIL
  RETURN "FAIL-RANDOM-BOUND"
 ENDIF
 IF OADS_RandomBytes(NIL)!=NIL
  RETURN "FAIL-RANDOM-BOUND"
 ENDIF
 IF OADS_RandomBytes(1000000000000)!=NIL
  RETURN "FAIL-RANDOM-BOUND"
 ENDIF
 cA:=OADS_RandomBytes(32)
 cB:=OADS_RandomBytes(32)
 IF cA==cB .OR. cA==Replicate(Chr(0),32)
  RETURN "FAIL-RANDOM-SMOKE"
 ENDIF
 IF Lower(hb_MD5("abc"))!="900150983cd24fb0d6963f7d28e17f72" .OR. Len(hb_MD5("abc",.T.))!=16
  RETURN "FAIL-MD5"
 ENDIF
 cKey:=hb_BlowfishKey(Replicate(Chr(0),8))
 cCipher:=hb_BlowfishEncrypt(cKey,Replicate(Chr(0),8),.T.)
 IF Lower(hb_StrToHex(cCipher))!="4ef997456198dd78" .OR. hb_BlowfishDecrypt(cKey,cCipher,.T.)!=Replicate(Chr(0),8)
  RETURN "FAIL-BLOWFISH-ECB"
 ENDIF
 cCipher:=hb_BlowfishEncrypt(cKey,"padded")
 IF hb_BlowfishDecrypt(cKey,cCipher)!="padded"
  RETURN "FAIL-BLOWFISH-PADDING"
 ENDIF
 cKey:=hb_BlowfishKey("01234567")
 cCipher:=hb_BlowfishEncrypt_CFB(cKey,"hello world",Replicate(Chr(0),8))
 IF Lower(hb_StrToHex(cCipher))!="d67db181ca6e6470f1deb5" .OR. hb_BlowfishDecrypt_CFB(cKey,cCipher,Replicate(Chr(0),8))!="hello world"
  RETURN "FAIL-BLOWFISH-CFB"
 ENDIF
 a:=Array(2)
 a[1]:=Round(Sqrt(81),0)
 a[2]:=Abs(-2)
 AAdd(a,3)
 h:=hb_Hash()
 hb_HSet(h,"answer",a[1]+a[2]+ATail(a))
 IF hb_HGet(h,"answer")!=14 .OR. Len(hb_HKeys(h))!=1 .OR. Year(hb_SToD("20260115"))!=2026 .OR. ValType(h)!="H"
  RETURN "FAIL-PURE-HELPERS"
 ENDIF
RETURN "CRYPTO-PASS|SHA256/384/512|HMAC256/384/512|binary/hex|random1..1024|invalid=NIL|MD5|BlowfishECB/CFB|purehelpers"
