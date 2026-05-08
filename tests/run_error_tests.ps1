# PowerShell 脚本
# 执行命令行：powershell -ExecutionPolicy Bypass -File tests\run_error_tests.ps1
$lexerExe = "tests\lexer_test.exe"
$errorDir = "tests\testcases\errorcases\lexer"

if (-not (Test-Path $lexerExe)) {
    Write-Host "Error: $lexerExe not found. Please compile first." -ForegroundColor Red
    Write-Host "Command: g++ -std=c++17 -I src tests/lexer_test.cpp src/lexer/lexer.cpp src/common/error.cpp -o tests/lexer_test.exe"
    exit 1
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Running Lexer Error Tests" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# 获取所有 .pas 文件并排序
$files = Get-ChildItem -Path $errorDir -Filter "*.pas" | Sort-Object Name

foreach ($file in $files) {
    Write-Host "----------------------------------------" -ForegroundColor Yellow
    Write-Host "Testing: $($file.Name)" -ForegroundColor Yellow
    Write-Host "----------------------------------------" -ForegroundColor Yellow
    & $lexerExe $file.FullName
    Write-Host ""
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Error Tests Complete" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan