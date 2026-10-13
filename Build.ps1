param([string]$SmartwatchRoot=(Join-Path $PSScriptRoot '..\Smartwatch'),[switch]$SkipSdkBuild)
$ErrorActionPreference='Stop'
if(!$SkipSdkBuild){& (Join-Path $SmartwatchRoot 'Build.ps1');if($LASTEXITCODE){throw 'Shared SDK build failed'}}
$plugins='C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins'
$gnu=Get-ChildItem -LiteralPath $plugins -Directory | Where-Object Name -Like 'com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*' | Sort-Object Name -Descending | Select-Object -First 1
$bin=Join-Path $gnu.FullName 'tools\bin'
$flags=@('-mcpu=cortex-m33','-mthumb','-mfpu=fpv5-sp-d16','-mfloat-abi=hard','-O2','-g','-ffunction-sections','-fdata-sections','-Wall','-Wextra','-DSTM32U5A9xx','-DUSE_HAL_DRIVER','-DLV_CONF_INCLUDE_SIMPLE','--specs=nano.specs','-Isrc')
foreach($inc in @('Core/Inc','App','Middlewares/NemaGFX/include','Middlewares','Middlewares/lvgl','Drivers/CMSIS/Include','Drivers/CMSIS/Device/ST/STM32U5xx/Include','Drivers/STM32U5xx_HAL_Driver/Inc','Drivers/BSP/STM32U5x9J-DK','Drivers/BSP/Components/Common')){$flags+='-I'+(Join-Path $SmartwatchRoot $inc)}
New-Item -ItemType Directory -Path (Join-Path $PSScriptRoot 'build') -Force | Out-Null
Push-Location -LiteralPath $PSScriptRoot
try{
 $objects=@();foreach($source in @('src/main.c','src/scene.c')){$obj='build/'+[IO.Path]::GetFileNameWithoutExtension($source)+'.o'; & (Join-Path $bin 'arm-none-eabi-gcc.exe') @flags -c $source -o $obj;if($LASTEXITCODE){throw "Compile failed: $source"};$objects+=$obj}
 $debug=Join-Path $SmartwatchRoot 'Debug'
 foreach($line in Get-Content (Join-Path $debug 'objects.list')){
   $rel=$line.Trim('"').Replace('./','')
   if($rel -eq 'Core/Src/main.o'){continue}
   if($rel.StartsWith('App/') -and $rel -notin @('App/platform/watch_display_stm32.o','App/platform/watch_gpu_stm32.o')){continue}
   $objects+=Join-Path $debug $rel
 }
 $argsLink=@(('-T'+(Join-Path $SmartwatchRoot 'STM32U5A9NJHXQ_FLASH.ld')),'-Wl,--gc-sections,-z,noexecstack,-Map=build/Duello.map','--specs=nosys.specs',('-L'+(Join-Path $SmartwatchRoot 'Middlewares/NemaGFX/lib/core/cortex_m33_revC/gcc')),'-lnemagfx-float-abi-hard','-lm')
 # GCC response file handles the many LVGL object paths and Unicode directories.
 $response=Join-Path $PSScriptRoot 'build/link.rsp'
 $quoted=$objects|ForEach-Object {'"'+$_.Replace('\','/')+'"'}
 [IO.File]::WriteAllLines($response,$quoted,[Text.UTF8Encoding]::new($false))
 & (Join-Path $bin 'arm-none-eabi-gcc.exe') @flags ('@'+$response) @argsLink -o 'build/Duello.elf'
 if($LASTEXITCODE){throw 'Duello link failed'}
 New-Item -ItemType Directory -Path 'Firmware' -Force|Out-Null
 & (Join-Path $bin 'arm-none-eabi-objcopy.exe') -O binary 'build/Duello.elf' 'Firmware/Duello.bin'
 if($LASTEXITCODE){throw 'Objcopy failed'}
 & (Join-Path $bin 'arm-none-eabi-size.exe') 'build/Duello.elf'
}finally{Pop-Location}
