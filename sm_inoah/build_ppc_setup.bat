rmdir /q /s dist_ppc
mkdir dist_ppc
copy ARMRel\inoah_ppc_arm_rel.exe dist_ppc\inoah.exe
copy readme_ppc.txt dist_ppc
copy eula_ppc.txt dist_ppc
copy inoah_ppc.inf dist_ppc
copy inoah_ppc.ini dist_ppc
cd dist_ppc
"C:\Program Files\Windows CE Tools\wce300\Smartphone 2002\tools\CabwizSP.exe" inoah_ppc.inf
move inoah_ppc.CAB inoah_ppc_1_0.cab
@rem ezsetup_sm -l english -i inoah_sm.ini -r readme.txt -e eula.txt -o iNoah_Smartphone_setup.exe
ezsetup_sm -l english -i inoah_ppc.ini -r readme_ppc.txt -e eula_ppc.txt -o inoah_ppc_1_0_setup.exe
del *.txt *.ini *.inf inoah.exe