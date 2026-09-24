param(
    [string] $OutputDirectory = (Join-Path $PSScriptRoot '..\assets')
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $OutputDirectory -PathType Container)) {
    throw "Icon output directory does not exist: $OutputDirectory"
}

# Procedural vector artwork, rasterized with supersampling. Uses only the .NET
# Framework supplied with Windows; no image editor or downloaded assets needed.
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;

public static class CapsAsShiftIcon {
    private static GraphicsPath Rounded(float x, float y, float w, float h, float r) {
        GraphicsPath path = new GraphicsPath();
        float d = r * 2;
        path.AddArc(x, y, d, d, 180, 90);
        path.AddArc(x + w - d, y, d, d, 270, 90);
        path.AddArc(x + w - d, y + h - d, d, d, 0, 90);
        path.AddArc(x, y + h - d, d, d, 90, 90);
        path.CloseFigure();
        return path;
    }

    public static Bitmap Render(int size) {
        using (Bitmap large = new Bitmap(size * 4, size * 4, PixelFormat.Format32bppArgb)) {
            using (Graphics g = Graphics.FromImage(large)) {
                g.Clear(Color.Transparent);
                g.SmoothingMode = SmoothingMode.AntiAlias;
                g.PixelOffsetMode = PixelOffsetMode.HighQuality;
                g.ScaleTransform(large.Width / 256f, large.Height / 256f);

                using (GraphicsPath basePath = Rounded(18, 24, 220, 214, 44))
                using (SolidBrush baseBrush = new SolidBrush(Color.FromArgb(12, 28, 53))) {
                    g.FillPath(baseBrush, basePath);
                }
                using (GraphicsPath face = Rounded(18, 16, 220, 204, 44))
                using (LinearGradientBrush blue = new LinearGradientBrush(
                    new RectangleF(18, 16, 220, 204),
                    Color.FromArgb(44, 85, 147), Color.FromArgb(23, 56, 95), 90f))
                using (Pen border = new Pen(Color.FromArgb(82, 125, 180), 2f)) {
                    g.FillPath(blue, face);
                    g.DrawPath(border, face);
                }

                // Shift symbol: no letters or tiny details that disappear at 16px.
                PointF[] points = {
                    new PointF(128, 64), new PointF(186, 123),
                    new PointF(153, 123), new PointF(153, 181),
                    new PointF(103, 181), new PointF(103, 123),
                    new PointF(70, 123)
                };
                using (Pen white = new Pen(Color.FromArgb(247, 251, 255), 13f)) {
                    white.LineJoin = LineJoin.Round;
                    g.DrawPolygon(white, points);
                }
            }

            Bitmap result = new Bitmap(size, size, PixelFormat.Format32bppArgb);
            using (Graphics g = Graphics.FromImage(result)) {
                g.CompositingMode = CompositingMode.SourceCopy;
                g.InterpolationMode = InterpolationMode.HighQualityBicubic;
                g.PixelOffsetMode = PixelOffsetMode.HighQuality;
                g.DrawImage(large, new Rectangle(0, 0, size, size),
                            0, 0, large.Width, large.Height, GraphicsUnit.Pixel);
            }
            return result;
        }
    }

    public static void Generate(string directory) {
        int[] sizes = { 16, 20, 24, 32, 40, 48, 64, 128, 256 };
        List<byte[]> frames = new List<byte[]>();
        foreach (int size in sizes) {
            using (Bitmap image = Render(size))
            using (MemoryStream stream = new MemoryStream()) {
                image.Save(stream, ImageFormat.Png);
                frames.Add(stream.ToArray());
            }
        }

        using (BinaryWriter writer = new BinaryWriter(
            File.Create(Path.Combine(directory, "caps-as-shift.ico")))) {
            writer.Write((ushort)0);
            writer.Write((ushort)1);
            writer.Write((ushort)sizes.Length);
            uint offset = (uint)(6 + 16 * sizes.Length);
            for (int i = 0; i < sizes.Length; i++) {
                byte dimension = sizes[i] == 256 ? (byte)0 : (byte)sizes[i];
                writer.Write(dimension);
                writer.Write(dimension);
                writer.Write((byte)0);
                writer.Write((byte)0);
                writer.Write((ushort)1);
                writer.Write((ushort)32);
                writer.Write((uint)frames[i].Length);
                writer.Write(offset);
                offset += (uint)frames[i].Length;
            }
            foreach (byte[] frame in frames) writer.Write(frame);
        }
        using (Bitmap preview = Render(512)) {
            preview.Save(Path.Combine(directory, "caps-as-shift.png"), ImageFormat.Png);
        }
    }
}
'@

[CapsAsShiftIcon]::Generate([System.IO.Path]::GetFullPath($OutputDirectory))
'Generated CapsAsShift icon (16, 20, 24, 32, 40, 48, 64, 128, 256 px) and PNG preview.'
