/* No DB/RDD/network access. Link the kit's Harbour wrapper and matching DLL. */
PROCEDURE Main( cMode )
   LOCAL nErrors := 0, bOld := ErrorBlock( {|e| nErrors++, Break(e) } )
   LOCAL lOn := cMode == "on"
   IF OAds_TraceMark("BEFORE_MASTER") != .F.
      ErrorLevel(1)
      QUIT
   ENDIF
   OAds_SetLogging(.T.)
   IF OAds_TraceMark("HB_SAVE_BEGIN") != lOn .OR. ;
      OAds_TraceMark(Replicate("L",120)) != lOn
      ErrorLevel(2)
      QUIT
   ENDIF
   IF OAds_TraceMark() != .F. .OR. OAds_TraceMark(NIL) != .F. .OR. ;
      OAds_TraceMark(123) != .F. .OR. OAds_TraceMark({}) != .F. .OR. ;
      OAds_TraceMark("") != .F. .OR. OAds_TraceMark(Replicate("X",121)) != .F. .OR. ;
      OAds_TraceMark("A"+Chr(0)+"B") != .F. .OR. ;
      OAds_TraceMark("A"+Chr(9)+"B") != .F. .OR. ;
      OAds_TraceMark("A"+Chr(10)+"B") != .F. .OR. ;
      OAds_TraceMark("A"+Chr(13)+"B") != .F.
      ErrorLevel(3)
      QUIT
   ENDIF
   IF OAds_TraceMark("HB_SAVE_END") != lOn
      ErrorLevel(4)
      QUIT
   ENDIF
   OAds_SetLogging(.F.)
   IF OAds_TraceMark("AFTER_MASTER") != .F. .OR. nErrors != 0
      ErrorLevel(5)
      QUIT
   ENDIF
   ErrorBlock(bOld)
   OutStd("HARBOUR-TRACE-MARKER-PASS"+hb_eol())
RETURN
