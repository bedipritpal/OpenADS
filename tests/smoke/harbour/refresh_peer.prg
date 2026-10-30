/* Synthetic Windows actual Harbour RDDADS / independent DBFCDX writer. */
REQUEST ADS, ADSCDX, DBFCDX
PROCEDURE Main( cMode, cUri, cDir, cValue )
 LOCAL h:=0, n, cOut, cCmd, nRc
 ErrorBlock( {|e| Fail( "runtime "+e:Description ) } )
 IF cMode == "writer"
  DbUseArea(.T.,"DBFCDX",cDir+"fresh.dbf","W",.T.)
  Assert(!NetErr(),"writer open")
  DbGoto(3)
  Assert(RLock(),"writer lock")
  FIELD->CITY:=cValue
  DbCommit(); DbUnlock(); DbCloseAll()
  ErrorLevel(0)
  RETURN
 ENDIF
 DbCreate(cDir+"fresh",{{"CODE","C",10,0},{"CITY","C",20,0}},"DBFCDX")
 DbUseArea(.T.,"DBFCDX",cDir+"fresh","SEED",.F.)
 FOR n:=1 TO 8
  DbAppend(); FIELD->CODE:="K"+StrZero(n,5); FIELD->CITY:="OLD"
 NEXT
 OrdCreate(,"CODE","CODE",{||FIELD->CODE},.F.)
 DbCommit(); DbCloseArea()
 AdsSetFileType(2)
 Assert(AdsConnect60(cUri,2,NIL,NIL,0,@h),"remote connect")
 DbUseArea(.T.,"ADSCDX","fresh.dbf","R",.T.,.F.,NIL,h)
 Assert(!NetErr(),"reader open")
 OrdSetFocus(1); DbGoTop(); DbSkip(2)
 Assert(RecNo()==3,"reader initial position")
 FOR EACH cValue IN {"PEER1","PEER2"}
  cOut:=cDir+"writer-"+cValue+".txt"
  cCmd:='"'+hb_ProgName()+'" writer unused "'+cDir+'" '+cValue+' > "'+cOut+'" 2>&1'
  nRc:=hb_run(cCmd)
  Assert(nRc==0,"independent DBFCDX writer exit")
  DbSelectArea("R")
  DbSkip(0)
  Assert(AllTrim(FIELD->CITY)==cValue,"RDDADS explicit refresh sees "+cValue)
  Assert(RecNo()==3 .AND. !Bof() .AND. !Eof(),"refresh position")
  DbSkip(1); Assert(RecNo()==4,"next record")
  DbSkip(-1); Assert(RecNo()==3,"reverse record")
 NEXT
 DbCloseAll(); AdsDisconnect(h)
 ? "REFRESH-PEER-HARBOUR-PASS"
 ErrorLevel(0)
 RETURN
STATIC PROCEDURE Assert(lOk,cWhat)
 IF !lOk
  Fail(cWhat)
 ENDIF
 RETURN
STATIC PROCEDURE Fail(cWhat)
 ? "REFRESH-PEER-HARBOUR-FAIL",cWhat
 ErrorLevel(1)
 QUIT
 RETURN
