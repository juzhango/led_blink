from building import *

cwd     = GetCurrentDir()
src     = Glob('*.c')
CPPPATH = [cwd]

group = DefineGroup('jz_led_blink', src, depend = [''], CPPPATH = CPPPATH)

Return('group')
