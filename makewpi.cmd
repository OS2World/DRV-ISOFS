/**/
wpistem = 'isofs110'
filetime = '01:10:00'
wisfile = 'isofs.wis'
packages.0 = 4
packages.stub = ''
packages.1.id = 1
packages.1.dir = 'bin'
packages.1.mask = '*.exe *.ico *.ifs'
packages.2.id = 2
packages.2.dir = 'bin'
packages.2.mask = '*.inf *.txt'
packages.3.id = 3
packages.3.dir = 'bin'
packages.3.mask = '*.msg'
packages.4.id = 4
packages.4.dir = 'source'
packages.4.mask = '*'
/**/
call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
call SysLoadFuncs
parse arg reldir
if reldir = '' then do
  say 'Usage: makewpi <release directory>'
  return
end
makedir = directory()
reldir = directory(reldir)
if reldir = '' then do
  say 'Invalid release directory'
  return
end
warpindir = SysIni(USER, 'WarpIN', 'Path')
if warpindir = 'ERROR:' | warpindir = '' then do
  say 'WarpIN is not installed correctly'
  return
end
filedate = date('S')
filedate = left(filedate,4)'-'substr(filedate,5,2)'-'substr(filedate,7,2)
call directory warpindir
wic1 = '@wic' reldir'\'wpistem '-a'
wic2 = ''
do i = 1 to packages.0
  wic2 = wic2 packages.i.id '-r -c'reldir'\'packages.i.dir packages.i.mask
  call setfiletime reldir'\'packages.i.dir
end
wic3 = packages.stub '-s' makedir'\'wisfile
wpifile = reldir'\'wpistem'.wpi'
call SysFileDelete wpifile
wic1 wic2 wic3
call SysSetFileDateTime wpifile, filedate, filetime
call directory makedir
return

setfiletime: procedure expose filedate filetime
  parse arg pkgdir
  call SysFileTree pkgdir'\*', 'stem', 'FOS'
  do i = 1 to stem.0
    call SysSetFileDateTime stem.i, filedate, filetime
  end
  return
