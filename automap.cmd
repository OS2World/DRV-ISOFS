/* Auto(Un)Mount program for ISO images */
signal on halt name halt
signal on syntax name error
signal on error name error
signal on failure name error
call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
call SysLoadFuncs

parse source . . me
pos = lastpos('\', me)
if pos = 0 then
  mypath = ''
else
  mypath = substr(me, 1, pos)

mapiso = '@'||mypath||'mapiso'
queue_name = ''
if arg() = 0 then do
  say 'Usage: isomount <filename.ISO>'
  call beep 400, 90
  call beep 300, 90
  call beep 200, 90
  call SysSleep 2
  return(-1)
end

ISOName = '"'||arg(1)||'"'
parse upper var ISOName UpperISOName
queue_name = rxqueue('Create')
call rxqueue 'Set', queue_name
mapiso '| rxqueue' queue_name
if rc <> 0 then do
  call rxqueue 'Delete', queue_name
  queue_name = ''
  call beep 400, 90
  call beep 300, 90
  call beep 200, 90
  call SysSleep 2
  return(-1)
end

ret = -2
do queued()
  parse upper pull MapDrive . MapISOName
  if UpperISOName = '"'||MapISOName||'"' then do
    mapiso MapDrive '-d'
    if rc=0 then do
      call beep 200, 90
      call beep 300, 90
      call beep 400, 90
      call SysSleep 2
      ret = 0
      leave
    end; else do
      ret = -1
      leave
    end
  end
end

if ret = -2 then do
  ret = -1
  FreeDrive = word(SysDriveMap(,'FREE'), 1)
  if FreeDrive = '' then
    say 'No Free Drive letters'
  else do
    mapiso FreeDrive ISOName
    if rc=0 then do
      call beep 200, 90
      call beep 300, 90
      call beep 400, 90
      call SysSleep 2
      ret = 0
    end
  end
end

if ret = -1 then do
  call beep 400, 90
  call beep 300, 90
  call beep 200, 90
  call SysSleep 2
end

call rxqueue 'Delete', queue_name
return(ret)

error:
  call beep 200, 250
  signal halt

halt:
  if queue_name <> '' then
    call rxqueue 'Delete', queue_name

  exit(-1)

