rmdir /q /s dist
mkdir dist
copy ARMRel\inoah_sm_arm_rel.exe dist\inoah.exe
copy readme_sm.txt dist\readme.txt
copy eula_sm.txt dist\eula.txt
copy inoah_sm.inf dist
copy inoah_sm.ini dist
cd dist
"C:\Program Files\Windows CE Tools\wce300\Smartphone 2002\tools\CabwizSP.exe" inoah_sm.inf
move inoah_sm.CAB inoah_sm_1_0.cab
@rem ezsetup -l english -i inoah_sm.ini -r readme.txt -e eula.txt -o iNoah_Smartphone_setup.exe
ezsetup -l english -i inoah_sm.ini -r readme.txt -e eula.txt -o iNoah_Smartphone_setup.exe
