<#
    Keil C51 命令行编译包装脚本

    为什么需要它：
      UV4.exe 属于 Windows GUI 子系统程序。用 PowerShell 的 & 调用、
      或者 VS Code 的 shell task 直接调用，都不会等待它执行完毕，
      拿不到退出码，也读不到编译日志。必须用 Start-Process -Wait。

    用法：
      build.ps1 -SearchFrom "C:\...\2-2LED闪烁"     # 从该目录向上自动查找 .uvproj
      build.ps1 -Project   "C:\...\按键消抖.uvproj"  # 直接指定工程
      build.ps1 -Project   "..." -Mode rebuild       # 全部重新编译
      build.ps1 -Project   "..." -Mode clean         # 清理产物
#>
[CmdletBinding()]
param(
    [string]$Project,
    [string]$SearchFrom,
    [ValidateSet('build', 'rebuild', 'clean')]
    [string]$Mode = 'build',
    [string]$Uv4 = 'G:\Keil5\UV4\UV4.exe'
)

$ErrorActionPreference = 'Stop'

# ---- 1. 定位工程文件 ----
if (-not $Project) {
    if (-not $SearchFrom) { $SearchFrom = (Get-Location).Path }
    $d = $SearchFrom
    while ($d) {
        $hit = @(Get-ChildItem -Path $d -Filter '*.uvproj' -File -ErrorAction SilentlyContinue |
                 Where-Object { $_.Length -gt 0 })
        if ($hit.Count -gt 0) { $Project = $hit[0].FullName; break }
        $parent = Split-Path $d -Parent
        if (-not $parent -or $parent -eq $d) { break }
        $d = $parent
    }
}

if (-not $Project) {
    Write-Host "[错误] 未能定位 .uvproj 工程文件。" -ForegroundColor Red
    Write-Host "       请用 -Project 参数显式指定，或从工程目录内发起编译。" -ForegroundColor Red
    exit 2
}

if (-not (Test-Path $Project)) {
    Write-Host "[错误] 找不到工程文件：$Project" -ForegroundColor Red
    exit 2
}

$len = (Get-Item $Project).Length
if ($len -eq 0) {
    Write-Host "[错误] 工程文件是 0 字节，已损坏：$Project" -ForegroundColor Red
    Write-Host "       Keil 无法打开空工程，请从备份恢复或重新新建工程。" -ForegroundColor Red
    exit 3
}

if (-not (Test-Path $Uv4)) {
    Write-Host "[错误] 找不到 UV4.exe：$Uv4" -ForegroundColor Red
    exit 2
}

# ---- 2. 提醒 Keil 单实例冲突 ----
$running = @(Get-Process -Name 'UV4' -ErrorAction SilentlyContinue)
if ($running.Count -gt 0) {
    Write-Host "[警告] 检测到 Keil μVision 正在运行（PID: $($running.Id -join ', ')）" -ForegroundColor Yellow
    Write-Host "       Keil 是单实例程序，已打开同类工程时命令行编译可能无效。" -ForegroundColor Yellow
    Write-Host "       若结果异常，请先关闭 Keil 窗口再试。" -ForegroundColor Yellow
    Write-Host ""
}

# ---- 3. 执行编译 ----
$flag = @{ build = '-b'; rebuild = '-r'; clean = '-c' }[$Mode]
$projDir = Split-Path $Project -Parent
$log = Join-Path $projDir 'build.log'
if (Test-Path $log) { Remove-Item $log -Force -ErrorAction SilentlyContinue }

Write-Host "工程: $Project"
Write-Host "模式: $Mode  ($flag)"
Write-Host ("-" * 60)

$sw = [System.Diagnostics.Stopwatch]::StartNew()
$proc = Start-Process -FilePath $Uv4 `
                      -ArgumentList @($flag, $Project, '-j0', '-o', $log) `
                      -Wait -PassThru
$sw.Stop()

# ---- 4. 回显日志（供 VS Code 的 problemMatcher 解析）----
if (Test-Path $log) {
    Get-Content $log -Encoding Default | ForEach-Object { Write-Host $_ }
} else {
    Write-Host "(未生成 build.log)"
}

$code = $proc.ExitCode
$meaning = switch ($code) {
    0  { '成功，无错误无警告' }
    1  { '成功，但有警告' }
    2  { '存在错误' }
    3  { '致命错误' }
    11 { '无法打开工程文件' }
    12 { '器件不支持' }
    13 { '写入文件错误' }
    15 { '许可证错误' }
    20 { '未知错误' }
    default { '未知状态' }
}

Write-Host ("-" * 60)
Write-Host ("退出码 {0} — {1}    耗时 {2} 秒" -f $code, $meaning, [math]::Round($sw.Elapsed.TotalSeconds, 1))

# ---- 5. 报告 hex 产物 ----
$hex = Get-ChildItem -Path $projDir -Filter '*.hex' -Recurse -ErrorAction SilentlyContinue |
       Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($hex -and $code -le 1) {
    Write-Host ("HEX: {0}  ({1} 字节)" -f $hex.FullName, $hex.Length) -ForegroundColor Green
}

exit $code