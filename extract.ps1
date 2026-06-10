$outFile = "c:\Users\Meghna Ashwin\Desktop\Miniproject\prompt.txt"
$files = @(
  "C:\Users\Meghna Ashwin\.gemini\antigravity-ide\brain\25923a57-fdc0-4a28-93ab-6a6bd0189ebd\.system_generated\logs\transcript.jsonl",
  "C:\Users\Meghna Ashwin\.gemini\antigravity-ide\brain\d55d0651-a1b4-4e99-a0b9-5753d38262f0\.system_generated\logs\transcript.jsonl"
)
$result = @()
foreach ($file in $files) {
    if (Test-Path $file) {
        $lines = Get-Content $file -Encoding UTF8
        foreach ($line in $lines) {
            try {
                $json = $line | ConvertFrom-Json
                if ($json.type -eq "USER_INPUT" -and $json.source -eq "USER_EXPLICIT") {
                    $result += "=== USER PROMPT ==="
                    $content = $json.content
                    $idx = $content.IndexOf("</USER_REQUEST>")
                    if ($idx -ge 0) {
                        $content = $content.Substring(0, $idx)
                    }
                    $content = $content -replace "<USER_REQUEST>\r?\n?", ""
                    $result += $content
                    $result += "`n"
                } elseif ($json.type -eq "PLANNER_RESPONSE" -and $json.source -eq "MODEL") {
                    if ($json.content) {
                        $result += "=== AI RESPONSE ==="
                        $result += $json.content
                        $result += "`n"
                    }
                }
            } catch {}
        }
    }
}
$result | Set-Content -Path $outFile -Encoding UTF8
