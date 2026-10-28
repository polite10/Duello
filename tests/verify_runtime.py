"""Validate the linked Duello runtime replies with the current shared DMA API.

Uses Unicorn and pyelftools; no UART device or physical board is accessed.
"""
import argparse
import json
from pathlib import Path
import sys
import struct

ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--smartwatch-root',type=Path);a=p.parse_args()
candidates=[a.smartwatch_root] if a.smartwatch_root else [ROOT.parent/name for name in ('smart_watch-main','Smartwatch','smart_watch')]
sdk=next((path for path in candidates if path and (path/'Core/Inc/device_runtime.h').is_file()),None)
if sdk:sys.path.insert(0,str(sdk/'tmp/simulator-libs'))
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE
from unicorn.arm_const import UC_CPU_ARM_CORTEX_M33, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC
from elftools.elf.elffile import ELFFile

u=Uc(UC_ARCH_ARM,UC_MODE_THUMB|UC_MODE_MCLASS);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_M33)
for address,size in [(0x08000000,0x400000),(0x20000000,0x280000),(0x0fff0000,0x1000),(0xe000e000,0x2000)]:u.mem_map(address,size)
with (ROOT/'build/Duello.elf').open('rb') as source:
    elf=ELFFile(source)
    symbols={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
    for segment in elf.iter_segments():
        if segment['p_type']=='PT_LOAD':u.mem_write(segment['p_vaddr'],segment.data())
def tick(*args):
    u.reg_write(UC_ARM_REG_R0,100);u.reg_write(UC_ARM_REG_PC,u.reg_read(UC_ARM_REG_LR))
address=symbols['HAL_GetTick']&~1;u.hook_add(UC_HOOK_CODE,tick,begin=address,end=address)
stack=0x2026f000
def call(name,*args):
    for register,value in zip((UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3),args[:4]):u.reg_write(register,value)
    for index,value in enumerate(args[4:]):u.mem_write(stack+index*4,struct.pack('<I',value))
    u.reg_write(UC_ARM_REG_SP,stack);u.reg_write(UC_ARM_REG_LR,0x0fff0001)
    u.emu_start(symbols[name]|1,0x0fff0000,count=1000000)
    assert u.reg_read(UC_ARM_REG_PC)==0x0fff0000,'Runtime did not return'
    return u.reg_read(UC_ARM_REG_R0)
app_name=0x20260000;command=app_name+128;output=app_name+256;boot=app_name+1024
checks=[]
def check(ok,message):
    assert ok,message
    checks.append(message);print('PASS: '+message,flush=True)
def reply(cmd,seq=1):
    u.mem_write(command,cmd.encode()+b'\0')
    length=call('device_runtime_format_reply',seq,command,output,512,boot)
    assert 0<length<512,'Reply was truncated by DMA buffer capacity'
    raw=bytes(u.mem_read(output,length));assert raw.endswith(b'\r\n')
    return json.loads(raw)
u.mem_write(app_name,b'Duello\0');call('device_runtime_init',app_name,symbols['duello_command'])
r=reply('HELLO')
check(r['app']=='Duello' and r['id']==2 and r['version']==1 and r['state']['ammo']==6,'Duello HELLO includes descriptor identity and full state')
check(reply('FIRE')['state']['error']=='draw_first','Application handler rejects FIRE before drawing')
check(reply('DRAW')['state']['action']=='draw','Current two-argument runtime dispatches DRAW')
check(reply('FIRE')['state']['error']=='busy','Busy action rejection survives shared runtime transport')
check(reply('BOOT')['error']=='bootloader_missing' and not u.mem_read(boot,1)[0],'BOOT reports a missing loader without reset')
info=struct.pack('<8I',0x44425631,2,0x0800A000,0x083FC000-0x0800A000,1,0x481,921600,0xfffffffe)
u.mem_write(0x08007f00,info)
check(reply('STATUS')['boot_version']==2,'Runtime reports installed bootloader revision 2')
r=reply('BOOT')
check(r['error']=='' and u.mem_read(boot,1)[0]==1,'BOOT uses the existing deferred reset contract')
# A future firmware supplies its own descriptor and name; no runtime edits needed.
u.mem_write(symbols['device_app_info'],struct.pack('<4I',0x44415631,3,7,1))
u.mem_write(app_name,b'ThirdApp\0');call('device_runtime_init',app_name,symbols['duello_command'])
r=reply('HELLO',2147483647)
check(r['app']=='ThirdApp' and r['id']==3 and r['version']==7,'New firmware identity/version come from its own descriptor')
u.mem_write(app_name,b'A'*48+b'\0');call('device_runtime_init',app_name,symbols['duello_command'])
check(len(reply('STATUS')['app'])==48,'Maximum package name and application state fit the shared 512-byte DMA reply')
(ROOT/'build/verification-local.json').write_text(json.dumps({'scope':'ARM ELF simulation; no physical board',
    'passed':len(checks),'failed':0,'checks':checks},indent=2)+'\n')
print(f'{len(checks)} ARM runtime checks passed.')
