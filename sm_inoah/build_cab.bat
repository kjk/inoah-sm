rmdir /q /s dist
mkdir dist
copy ARMRel\sm_inoah.exe dist\inoah.exe
copy inoah.inf dist
cd dist
"C:\Program Files\Windows CE Tools\wce300\Smartphone 2002\tools\CabwizSP.exe" inoah.inf
move inoah.CAB inoah_1_0.CAB
dir
