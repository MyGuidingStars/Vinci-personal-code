<#
    一键新建 / 补建 51 单片机 Keil 工程（支持多文件）

    用法：
      .\new-project.ps1 -Name "5-2定时器中断"              # 全新实验
      .\new-project.ps1 -Name "6-1串口" -NoCompile        # 只建工程不编译

    行为：
      - 目录不存在      → 创建
      - 目录里已有 .c   → 【全部】纳入工程，绝不覆盖你的代码
      - 目录里没有 .c   → 生成一个带主循环框架的模板
      - 目录里的 .h     → 一并纳入工程（方便查看，不参与编译）
      - 已有 .uvproj    → 拒绝执行，避免覆盖
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Name,

    [string]$Root = 'G:\51Projects',
    [string]$Template,
    [switch]$NoCompile
)

$ErrorActionPreference = 'Stop'

$Name = $Name.Trim()
if (-not $Name) {
    Write-Host "[错误] 工程名不能为空" -ForegroundColor Red
    exit 2
}
foreach ($ch in [System.IO.Path]::GetInvalidFileNameChars()) {
    if ($Name.Contains($ch)) {
        Write-Host "[错误] 工程名不能包含字符: $ch" -ForegroundColor Red
        exit 2
    }
}

if (-not $Template) { $Template = Join-Path $Root '.vscode\project-template.uvproj' }
if (-not (Test-Path $Template)) {
    Write-Host "[错误] 找不到工程模板: $Template" -ForegroundColor Red
    exit 2
}

$dir  = Join-Path $Root $Name
$proj = Join-Path $dir "$Name.uvproj"

if (Test-Path $proj) {
    Write-Host "[错误] 工程已存在: $proj" -ForegroundColor Red
    Write-Host "       为避免覆盖，已退出。" -ForegroundColor Red
    exit 3
}

# ---- 1. 目录 ----
if (-not (Test-Path $dir)) {
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    Write-Host "[1/4] 创建目录: $dir"
} else {
    Write-Host "[1/4] 目录已存在，沿用: $dir"
}

$utf8 = New-Object System.Text.UTF8Encoding($false)

# ---- 2. 收集源文件（全部 .c，无则生成模板）----
$cFiles = @(Get-ChildItem -Path $dir -Filter '*.c' -File -ErrorAction SilentlyContinue | Sort-Object Name)
$hFiles = @(Get-ChildItem -Path $dir -Filter '*.h' -File -ErrorAction SilentlyContinue | Sort-Object Name)

if ($cFiles.Count -eq 0) {
    $tplName = "$Name.c"
    $cLines = @(
        '#include <REGX52.H>',
        '',
        'void main()',
        '{',
        "`twhile(1)",
        "`t{",
        "`t`t",
        "`t}",
        '}',
        ''
    )
    [System.IO.File]::WriteAllText((Join-Path $dir $tplName), ($cLines -join "`r`n"), $utf8)
    $cFiles = @(Get-Item (Join-Path $dir $tplName))
    Write-Host "[2/4] 目录内无 .c，已生成模板: $tplName"
} else {
    Write-Host "[2/4] 发现 $($cFiles.Count) 个 .c 文件，将全部纳入工程："
    $cFiles | ForEach-Object { Write-Host "        $($_.Name)  ($($_.Length) 字节)" }
}
if ($hFiles.Count -gt 0) {
    Write-Host "      另有 $($hFiles.Count) 个 .h 文件：$(($hFiles | ForEach-Object { $_.Name }) -join ', ')"
}

# ---- 3. 生成工程，写入全部 .c 和 .h ----
$bytes  = [System.IO.File]::ReadAllBytes($Template)
$hasBom = ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
$xml = [System.IO.File]::ReadAllText($Template, $utf8)

$nodes = New-Object System.Collections.Generic.List[string]
foreach ($f in $cFiles) {
    $nodes.Add("            <File>`r`n              <FileName>$($f.Name)</FileName>`r`n              <FileType>1</FileType>`r`n              <FilePath>.\$($f.Name)</FilePath>`r`n            </File>")
}
foreach ($f in $hFiles) {
    $nodes.Add("            <File>`r`n              <FileName>$($f.Name)</FileName>`r`n              <FileType>5</FileType>`r`n              <FilePath>.\$($f.Name)</FilePath>`r`n            </File>")
}

$filesBlock = "          <Files>`r`n" + ($nodes -join "`r`n") + "`r`n          </Files>"
$xml = [regex]::Replace($xml, '<Files>[\s\S]*?</Files>', { param($m) $filesBlock })
$xml = [regex]::Replace($xml, '<OutputName>.*?</OutputName>', "<OutputName>$Name</OutputName>")

$enc = if ($hasBom) { New-Object System.Text.UTF8Encoding($true) } else { $utf8 }
[System.IO.File]::WriteAllText($proj, $xml, $enc)
Write-Host "[3/4] 生成工程: $proj"
Write-Host "       纳入 $($cFiles.Count) 个 .c + $($hFiles.Count) 个 .h"

# ---- 4. 编译验证 ----
if ($NoCompile) {
    Write-Host "[4/4] 跳过编译验证"
} else {
    Write-Host "[4/4] 编译验证..."
    Write-Host ""
    $build = Join-Path $Root '.vscode\build.ps1'
    if (Test-Path $build) {
        & $build -Project $proj
        Write-Host ""
    } else {
        Write-Host "[提示] 未找到 build.ps1，跳过编译" -ForegroundColor Yellow
    }
}

Write-Host ("=" * 58)
Write-Host "工程已就绪: $Name" -ForegroundColor Green
$cFiles | ForEach-Object { Write-Host "  C:  $(Join-Path $dir $_.Name)" }
$hFiles | ForEach-Object { Write-Host "  H:  $(Join-Path $dir $_.Name)" }
Write-Host "  HEX $(Join-Path $dir 'Objects')"
Write-Host ("=" * 58)