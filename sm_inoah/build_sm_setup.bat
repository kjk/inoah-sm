rmdir /q /s dist
mkdir dist
copy ARMRel\inoah_sm_arm_rel.exe dist\inoah.exe
copy inoah_sm.inf dist
cd dist
"C:\Program Files\Windows CE Tools\wce300\Smartphone 2002\tools\CabwizSP.exe" inoah_sm.inf
move inoah.CAB inoah_sm_1_0.cab
@rem ezsetup -l english -i inoah_sm.ini -r readme.txt -e eula.txt -o iNoah_Smartphone_setup.exe
ezsetup -l english -i inoah_sm.ini -o iNoah_Smartphone_setup.exe
