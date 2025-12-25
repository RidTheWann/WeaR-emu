# Create multi-size ICO from PNG - NO CROP (user already cropped)
Add-Type -AssemblyName System.Drawing

$pngPath = "C:\Users\RidTheWann\Documents\WeaR-emu\resources\wear_logo.png"
$icoPath = "C:\Users\RidTheWann\Documents\WeaR-emu\resources\wear_logo.ico"

# Load source PNG
$img = [System.Drawing.Image]::FromFile($pngPath)

# NO CROP - logo already fills entire frame
$cropX = 0
$cropY = 0
$cropWidth = $img.Width
$cropHeight = $img.Height

Write-Host "Using full image: ${cropWidth}x${cropHeight}"

# Create icon sizes
$sizes = @(256, 128, 64, 48, 32, 16)
$memoryStream = New-Object System.IO.MemoryStream

# ICO header
$bw = New-Object System.IO.BinaryWriter($memoryStream)
$bw.Write([UInt16]0)  # Reserved
$bw.Write([UInt16]1)  # Type (1 = ICO)
$bw.Write([UInt16]$sizes.Length)  # Number of images

$imageOffset = 6 + ($sizes.Length * 16)
$imageData = @()

foreach ($size in $sizes) {
    $bitmap = New-Object System.Drawing.Bitmap($size, $size)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    
    # High quality settings
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    
    # Draw cropped/zoomed portion
    $destRect = New-Object System.Drawing.Rectangle(0, 0, $size, $size)
    $srcRect = New-Object System.Drawing.Rectangle($cropX, $cropY, $cropWidth, $cropHeight)
    $graphics.DrawImage($img, $destRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
    
    $ms = New-Object System.IO.MemoryStream
    $bitmap.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $imageBytes = $ms.ToArray()
    
    # Directory entry (256 is represented as 0 in ICO format)
    $widthByte = if ($size -eq 256) { [byte]0 } else { [byte]$size }
    $heightByte = if ($size -eq 256) { [byte]0 } else { [byte]$size }
    
    $bw.Write($widthByte)   # Width
    $bw.Write($heightByte)  # Height
    $bw.Write([byte]0)      # Colors
    $bw.Write([byte]0)      # Reserved
    $bw.Write([UInt16]1)    # Planes
    $bw.Write([UInt16]32)   # Bits per pixel
    $bw.Write([UInt32]$imageBytes.Length)  # Size
    $bw.Write([UInt32]$imageOffset)        # Offset
    
    $imageData += $imageBytes
    $imageOffset += $imageBytes.Length
    
    $graphics.Dispose()
    $bitmap.Dispose()
    $ms.Dispose()
}

# Write image data
foreach ($data in $imageData) {
    $bw.Write($data)
}

$bw.Flush()
[System.IO.File]::WriteAllBytes($icoPath, $memoryStream.ToArray())

$bw.Close()
$memoryStream.Close()
$img.Dispose()

Write-Host "ICO created with ZOOMED controller (sizes: 256, 128, 64, 48, 32, 16)"
