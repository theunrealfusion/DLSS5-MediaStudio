Add-Type -AssemblyName System.Drawing

function Create-Icon {
    param([string]$OutputPath)
    
    $sizes = @(16, 32, 48, 64, 128, 256)
    $bitmaps = @()
    
    foreach ($size in $sizes) {
        $bmp = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
        $g.Clear([System.Drawing.Color]::Transparent)
        
        # Outer dark rounded shield / badge
        $pad = [Math]::Max(1, [int]($size * 0.05))
        $rect = New-Object System.Drawing.Rectangle($pad, $pad, $size - 2 * $pad, $size - 2 * $pad)
        $bgBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
            $rect,
            [System.Drawing.Color]::FromArgb(255, 15, 23, 42),
            [System.Drawing.Color]::FromArgb(255, 2, 6, 23),
            [System.Drawing.Drawing2D.LinearGradientMode]::ForwardDiagonal
        )
        $g.FillEllipse($bgBrush, $rect)
        $bgBrush.Dispose()
        
        # NVIDIA Green accent ring
        $penWidth = [Math]::Max(1.5, $size * 0.06)
        $ringPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 118, 185, 0), $penWidth)
        $g.DrawEllipse($ringPen, $pad + $penWidth / 2, $pad + $penWidth / 2, $size - 2 * $pad - $penWidth, $size - 2 * $pad - $penWidth)
        $ringPen.Dispose()
        
        # Cyber / DLSS 5 Neural Diamond in Center
        $cx = $size / 2.0
        $cy = $size / 2.0
        $r = $size * 0.28
        $points = @(
            New-Object System.Drawing.PointF($cx, $cy - $r),
            New-Object System.Drawing.PointF($cx + $r * 0.85, $cy),
            New-Object System.Drawing.PointF($cx, $cy + $r),
            New-Object System.Drawing.PointF($cx - $r * 0.85, $cy)
        )
        $diamondBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
            $rect,
            [System.Drawing.Color]::FromArgb(255, 0, 229, 255),
            [System.Drawing.Color]::FromArgb(255, 118, 185, 0),
            [System.Drawing.Drawing2D.LinearGradientMode]::Vertical
        )
        $g.FillPolygon($diamondBrush, $points)
        $diamondBrush.Dispose()
        
        # Central Core
        $coreR = [Math]::Max(2.0, $size * 0.10)
        $coreBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
        $g.FillEllipse($coreBrush, $cx - $coreR, $cy - $coreR, $coreR * 2, $coreR * 2)
        $coreBrush.Dispose()
        
        $g.Dispose()
        $bitmaps += $bmp
    }
    
    # Save as multi-resolution ICO file
    $fs = [System.IO.File]::OpenWrite($OutputPath)
    $bw = New-Object System.IO.BinaryWriter($fs)
    
    # ICONDIR structure
    $bw.Write([uint16]0) # Reserved
    $bw.Write([uint16]1) # Type (1 = ICO)
    $bw.Write([uint16]$bitmaps.Count) # Image count
    
    $pngStreams = @()
    foreach ($bmp in $bitmaps) {
        $ms = New-Object System.IO.MemoryStream
        $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
        $pngStreams += $ms
    }
    
    $offset = 6 + ($bitmaps.Count * 16)
    for ($i = 0; $i -lt $bitmaps.Count; $i++) {
        $bmp = $bitmaps[$i]
        $ms = $pngStreams[$i]
        
        $w = if ($bmp.Width -ge 256) { 0 } else { [byte]$bmp.Width }
        $h = if ($bmp.Height -ge 256) { 0 } else { [byte]$bmp.Height }
        
        $bw.Write([byte]$w)
        $bw.Write([byte]$h)
        $bw.Write([byte]0) # Color palette
        $bw.Write([byte]0) # Reserved
        $bw.Write([uint16]1) # Color planes
        $bw.Write([uint16]32) # Bits per pixel
        $bw.Write([uint32]$ms.Length) # Size of image data
        $bw.Write([uint32]$offset) # Offset
        
        $offset += $ms.Length
    }
    
    foreach ($ms in $pngStreams) {
        $bytes = $ms.ToArray()
        $bw.Write($bytes)
        $ms.Dispose()
    }
    
    $bw.Flush()
    $fs.Dispose()
    foreach ($bmp in $bitmaps) { $bmp.Dispose() }
    Write-Host "Created high-resolution icon: $OutputPath" -ForegroundColor Green
}

Create-Icon -OutputPath "RTX_DLSS5_Studio\app.ico"
