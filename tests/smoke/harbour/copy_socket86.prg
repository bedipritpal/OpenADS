/* Actual TCP regression for both CopyFile spellings, not direct dispatch.
 * Takes the URI of a disposable server with EnableFileFunc=1. */
PROCEDURE Main(cUri)
 LOCAL h:=Copy86Connect(cUri),f,cRead,cBody:=Replicate("A"+Chr(0)+Chr(255)+Chr(13)+Chr(10),26215),n
 ErrorBlock({|e| OutStd("COPY86 ERROR "+e:description+" "+e:operation+hb_eol()),ErrorLevel(1),__Quit()})
 Check(h>0,"connect")
 f:=OAds_FCreate(h,"copy86-source.bin",0)
 Check(f!=0,"create source")
 Check(OAds_FWrite(f,cBody)==Len(cBody),"write source")
 Check(OAds_FClose(f),"close source")
 Check(OAds_CopyFile(h,"copy86-source.bin","copy86-target.bin"),"CopyFile explicit connection")
 Check(OAds_FCopyFile(h,"copy86-source.bin","copy86-alias.bin"),"FCopyFile explicit connection")
 OAds_SetConnection(h)
 Check(OAds_FCopyFile("copy86-source.bin","copy86-default.bin"),"FCopyFile default connection")
 Check(OAds_CopyFile("copy86-source.bin","copy86-target.bin"),"overwrite existing target")
 FOR n:=1 TO 3
  f:=OAds_FOpen(h,{"copy86-target.bin","copy86-alias.bin","copy86-default.bin"}[n],3)
  Check(f!=0,"open copied file")
  cRead:=OAds_FRead(f,Len(cBody)+1)
  Check(cRead==cBody,"byte-for-byte content")
  Check(OAds_FClose(f),"close copied file")
 NEXT
 Check(!OAds_FCopyFile(h,"copy86-missing.bin","copy86-must-not-exist.bin"),"missing source fails")
 Check(!OAds_FCopyFile(h,"copy86-source.bin","../copy86-escape.bin"),"jail escape denied")
 FOR EACH cRead IN {"copy86-source.bin","copy86-target.bin","copy86-alias.bin","copy86-default.bin"}
  Check(OAds_DeleteFile(h,cRead),"cleanup")
 NEXT
 Copy86Disconnect(h)
 OutStd("COPY86-ACTUAL-TCP PASS"+hb_eol())
RETURN
STATIC PROCEDURE Check(lOk,cWhat)
 IF !lOk
  OutStd("COPY86 FAIL "+cWhat+hb_eol())
  ErrorLevel(1)
  QUIT
 ENDIF
RETURN
