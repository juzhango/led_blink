from building import *

cwd     = GetCurrentDir()
src     = Glob('*.c')
CPPPATH = [cwd]

# 如果启用了 LED Blink Demo，则加入 demo 源文件
if GetDepend('PKG_USING_LED_BLINK_DEMO'):
    demo_src = Glob('demo/led_blink_demo.c')
    src += demo_src

group = DefineGroup('led_blink', src, depend=['PKG_USING_LED_BLINK'], CPPPATH=CPPPATH)

Return('group')
