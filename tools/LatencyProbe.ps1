param(
  [Parameter(Mandatory=$true)]
  [string]$Targets,
  [int]$Count = 60,
  [int]$TimeoutMs = 1500,
  [string]$OutCsv = ".\latency_probe.csv"
)

$ErrorActionPreference = "Stop"

function Percentile {
  param([double[]]$Values, [double]$P)
  if ($Values.Count -eq 0) { return -1.0 }
  $sorted = $Values | Sort-Object
  if ($P -le 0.0) { return [double]$sorted[0] }
  if ($P -ge 1.0) { return [double]$sorted[$sorted.Count-1] }
  $idx = ($sorted.Count - 1) * $P
  $i0 = [int][math]::Floor($idx)
  $i1 = [int][math]::Ceiling($idx)
  if ($i0 -eq $i1) { return [double]$sorted[$i0] }
  $frac = $idx - $i0
  return [double]($sorted[$i0] + ($sorted[$i1] - $sorted[$i0]) * $frac)
}

function TcpConnectMs {
  param([string]$Host, [int]$Port, [int]$TimeoutMs)

  $sw = [System.Diagnostics.Stopwatch]::StartNew()
  try {
    $client = New-Object System.Net.Sockets.TcpClient
    $iar = $client.BeginConnect($Host, $Port, $null, $null)
    if (-not $iar.AsyncWaitHandle.WaitOne($TimeoutMs, $false)) {
      try { $client.Close() } catch {}
      return @{ ok = $false; ms = -1.0; err = "TIMEOUT" }
    }
    $client.EndConnect($iar)
    $sw.Stop()
    try { $client.Close() } catch {}
    return @{ ok = $true; ms = [double]$sw.Elapsed.TotalMilliseconds; err = "" }
  } catch {
    $sw.Stop()
    return @{ ok = $false; ms = -1.0; err = $_.Exception.Message }
  }
}

$targets = $Targets.Split(",") | ForEach-Object { $_.Trim() } | Where-Object { $_.Length -gt 0 }

$rows = New-Object System.Collections.Generic.List[object]

foreach ($t in $targets) {
  if ($t -notmatch ":") { throw "Target must be host:port -> $t" }
}

Write-Host "Probing $($targets.Count) targets for $Count samples each..."

foreach ($t in $targets) {
  $parts = $t.Split(":")
  $host = $parts[0]
  $port = [int]$parts[1]

  $vals = New-Object System.Collections.Generic.List[double]

  for ($i = 1; $i -le $Count; $i++) {
    $r = TcpConnectMs -Host $host -Port $port -TimeoutMs $TimeoutMs
    $ts = (Get-Date).ToString("s")
    $rows.Add([pscustomobject]@{
      timestamp = $ts
      target = $t
      ok = $r.ok
      ms = $r.ms
      err = $r.err
    }) | Out-Null

    if ($r.ok) { $vals.Add([double]$r.ms) | Out-Null }

    Start-Sleep -Milliseconds 250
  }

  $p50 = Percentile -Values ($vals.ToArray()) -P 0.50
  $p95 = Percentile -Values ($vals.ToArray()) -P 0.95
  $min = if ($vals.Count -gt 0) { ($vals | Measure-Object -Minimum).Minimum } else { -1.0 }
  $max = if ($vals.Count -gt 0) { ($vals | Measure-Object -Maximum).Maximum } else { -1.0 }

  Write-Host ""
  Write-Host "Target: $t"
  Write-Host "  ok: $($vals.Count)/$Count"
  Write-Host ("  p50: {0:N2} ms" -f $p50)
  Write-Host ("  p95: {0:N2} ms" -f $p95)
  Write-Host ("  min: {0:N2} ms" -f $min)
  Write-Host ("  max: {0:N2} ms" -f $max)
}

$rows | Export-Csv -NoTypeInformation -Encoding ASCII -Path $OutCsv
Write-Host ""
Write-Host "Wrote: $OutCsv"
