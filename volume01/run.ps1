# 第1冊の掲載コードを、Windows でビルドして実行するためのスクリプトです。
# ふだんは同じフォルダの run.bat をダブルクリックしてください。
#
# 直接呼ぶ場合:
#   powershell -NoProfile -ExecutionPolicy Bypass -File run.ps1       メニューを出す
#   powershell -NoProfile -ExecutionPolicy Bypass -File run.ps1 all   全部をビルドして実行する
#
# 生成した実行ファイルは一時フォルダ（%TEMP%\design-patterns-book）に置き、
# このフォルダの中には何も作りません。

param([string]$Mode = '')

$ErrorActionPreference = 'Continue'
try { [Console]::OutputEncoding = New-Object System.Text.UTF8Encoding $false } catch {}
$OutputEncoding = New-Object System.Text.UTF8Encoding $false

$Root      = Split-Path -Parent $MyInvocation.MyCommand.Path
$OnWindows = ($env:OS -eq 'Windows_NT')
$ExeName   = if ($OnWindows) { 'app.exe' } else { 'app' }
$BuildRoot = Join-Path ([System.IO.Path]::GetTempPath()) 'design-patterns-book'

$ChapterNames = @{
    'ch01-strategy' = '第1章 Strategy'
    'ch02-state'    = '第2章 State   '
    'ch03-observer' = '第3章 Observer'
}
$StateNames = @{
    '1-before'      = '変更前   フェーズ1の現状コード'
    '2-trial'       = '仮実装   フェーズ3で構造を変えずに変更を当てたもの'
    '3-after'       = '完成     フェーズ7の完成コード'
    '4-after-split' = '完成     実務のファイル分割版'
}

function Get-Targets {
    $list = @()
    Get-ChildItem -Path $Root -Directory | Where-Object { $_.Name -like 'ch*' } | Sort-Object Name | ForEach-Object {
        $ch = $_
        Get-ChildItem -Path $ch.FullName -Directory | Sort-Object Name | ForEach-Object {
            if (Test-Path (Join-Path $_.FullName 'main.cpp')) {
                $cn = $ChapterNames[$ch.Name]; if (-not $cn) { $cn = $ch.Name }
                $sn = $StateNames[$_.Name];   if (-not $sn) { $sn = $_.Name }
                $list += [pscustomobject]@{
                    Dir   = $_.FullName
                    Key   = ($ch.Name + '_' + $_.Name)
                    Path  = ($ch.Name + '\' + $_.Name)
                    Label = ($cn + '  ' + $sn)
                }
            }
        }
    }
    return ,$list
}

function Find-Compiler {
    foreach ($name in @('g++', 'clang++', 'cl')) {
        if (Get-Command $name -ErrorAction SilentlyContinue) { return $name }
    }
    return $null
}

function Show-NoCompiler {
    Write-Host ''
    Write-Host 'C++ コンパイラが見つかりませんでした。' -ForegroundColor Yellow
    Write-Host '次のどれか一つを入れて、コマンドから呼べるようにしてください（C++14 が使えれば十分です）。'
    Write-Host ''
    Write-Host '  g++      MSYS2 や WinLibs などの MinGW-w64 系。入れたあと bin フォルダを PATH へ追加します'
    Write-Host '  clang++  LLVM'
    Write-Host '  cl       Visual Studio の C++ 開発ツール。「Developer PowerShell for VS」から run.ps1 を実行します'
    Write-Host ''
    Write-Host '入れたあとで新しいウィンドウを開き、もう一度 run.bat を実行してください。'
    Write-Host '詳しい手順は、リポジトリ直下の README.md の「Windows での注意点」にあります。'
}

function Build-Target($t, $compiler) {
    $out = Join-Path $BuildRoot $t.Key
    New-Item -ItemType Directory -Force -Path $out | Out-Null
    $exe = Join-Path $out $ExeName
    if (Test-Path $exe) { Remove-Item $exe -Force }
    $srcs = @(Get-ChildItem -Path $t.Dir -Filter '*.cpp' | Sort-Object Name | ForEach-Object { $_.FullName })

    if ($compiler -eq 'cl') {
        Push-Location $out
        try { & cl /nologo /EHsc /std:c++14 /utf-8 /W3 ('/I' + $t.Dir) $srcs ('/Fe' + $exe) | Out-Host }
        finally { Pop-Location }
    } else {
        & $compiler -std=c++14 -Wall ('-I' + $t.Dir) $srcs -o $exe
    }
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $exe)) { return $null }
    return $exe
}

function Invoke-Target($exe, $dir) {
    Push-Location $dir
    try { $lines = @(& $exe) } finally { Pop-Location }
    return ,@($lines | ForEach-Object { ([string]$_).TrimEnd("`r") })
}

function Run-One($t, $compiler) {
    Write-Host ''
    Write-Host ('----- ' + $t.Label + ' -----') -ForegroundColor Cyan
    Write-Host ('フォルダ: ' + $t.Path)
    Write-Host ('ビルド中（' + $compiler + '）...')
    $exe = Build-Target $t $compiler
    if (-not $exe) {
        Write-Host 'ビルドに失敗しました。上のメッセージを確認してください。' -ForegroundColor Red
        return $false
    }
    Write-Host '実行結果:' -ForegroundColor Cyan
    Write-Host ''
    $lines = Invoke-Target $exe $t.Dir
    $lines | ForEach-Object { Write-Host $_ }
    Write-Host ''
    return $true
}

function Run-All($targets, $compiler) {
    Write-Host ''
    Write-Host ('全 ' + $targets.Count + ' 件をビルドして実行します（' + $compiler + '）。') -ForegroundColor Cyan
    Write-Host ''
    $ng = 0
    foreach ($t in $targets) {
        if (-not (Run-One $t $compiler)) { $ng++ }
    }
    Write-Host ''
    if ($ng -eq 0) {
        Write-Host ('すべて実行できました（' + $targets.Count + ' 件）。') -ForegroundColor Green
        return $true
    }
    Write-Host ($ng.ToString() + ' 件に問題があります。') -ForegroundColor Red
    return $false
}

function Show-Menu($targets, $compiler) {
    Write-Host ''
    Write-Host '=====================================================================' -ForegroundColor Cyan
    Write-Host (' 第1冊 掲載コードの実行      コンパイラ: ' + $compiler)
    Write-Host '=====================================================================' -ForegroundColor Cyan
    $prev = ''
    for ($i = 0; $i -lt $targets.Count; $i++) {
        $chap = $targets[$i].Key.Split('_')[0]
        if ($prev -ne '' -and $chap -ne $prev) { Write-Host '' }
        $prev = $chap
        Write-Host ('  {0,2}  {1}' -f ($i + 1), $targets[$i].Label)
    }
    Write-Host ''
    Write-Host '   A  全部をビルドして実行する'
    Write-Host '   Q  終了'
    Write-Host ''
}

# ---- ここから本体 ----
$compiler = Find-Compiler
if (-not $compiler) {
    Show-NoCompiler
    if ($Mode -ne 'all') { [void](Read-Host 'Enter キーで終了します') }
    exit 2
}

$targets = Get-Targets

if ($Mode -eq 'all') {
    if (Run-All $targets $compiler) { exit 0 } else { exit 1 }
}

while ($true) {
    Show-Menu $targets $compiler
    $ans = Read-Host '番号を入力して Enter'
    if ($null -eq $ans) { break }
    $ans = $ans.Trim()
    if ($ans -match '^[qQ]$') { break }
    if ($ans -match '^[aA]$') {
        [void](Run-All $targets $compiler)
        [void](Read-Host 'Enter キーでメニューへ戻ります')
        continue
    }
    $n = 0
    if ([int]::TryParse($ans, [ref]$n) -and $n -ge 1 -and $n -le $targets.Count) {
        [void](Run-One $targets[$n - 1] $compiler)
        [void](Read-Host 'Enter キーでメニューへ戻ります')
    } elseif ($ans -ne '') {
        Write-Host '番号が正しくありません。' -ForegroundColor Yellow
    }
}
