Add-Type -AssemblyName System.Drawing

$src = 'C:\Users\victo\.gemini\antigravity\brain\362a2c49-b975-4d99-a018-bc07e46690e4\re3_nemesis_icon_1791342306262.jpg'
$img = [System.Drawing.Image]::FromFile($src)

$sizes = @(256, 128, 64, 48, 32, 16)
$pngBytesList = @()

foreach ($sz in $sizes) {
    $bmp = New-Object System.Drawing.Bitmap($sz, $sz)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.DrawImage($img, 0, 0, $sz, $sz)
    $g.Dispose()
    
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $pngBytesList += ,$ms.ToArray()
    $ms.Dispose()
    $bmp.Dispose()
}
$img.Dispose()

function Save-Ico($outPath) {
    $fs = [System.IO.File]::Create($outPath)
    $bw = New-Object System.IO.BinaryWriter($fs)
    
    # ICONDIR
    $bw.Write([uint16]0) # Reserved
    $bw.Write([uint16]1) # Type (1 = icon)
    $bw.Write([uint16]$sizes.Count) # Image count
    
    $offset = 6 + ($sizes.Count * 16)
    
    for ($i = 0; $i -lt $sizes.Count; $i++) {
        $sz = $sizes[$i]
        $w = if ($sz -ge 256) { [byte]0 } else { [byte]$sz }
        $h = if ($sz -ge 256) { [byte]0 } else { [byte]$sz }
        $bytes = $pngBytesList[$i]
        
        $bw.Write([byte]$w)
        $bw.Write([byte]$h)
        $bw.Write([byte]0) # Color count
        $bw.Write([byte]0) # Reserved
        $bw.Write([uint16]1) # Color planes
        $bw.Write([uint16]32) # Bits per pixel
        $bw.Write([uint32]$bytes.Length) # Size of image data
        $bw.Write([uint32]$offset) # Offset of image data
        
        $offset += $bytes.Length
    }
    
    for ($i = 0; $i -lt $sizes.Count; $i++) {
        $bw.Write($pngBytesList[$i])
    }
    
    $bw.Close()
    $fs.Close()
}

Save-Ico 'launcher\re3.ico'
Save-Ico 'RE3Recomp\build\re3.ico'
Write-Output 'Created launcher\re3.ico and RE3Recomp\build\re3.ico successfully!'
