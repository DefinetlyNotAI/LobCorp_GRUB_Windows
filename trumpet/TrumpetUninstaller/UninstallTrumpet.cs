using System;
using System.Diagnostics;
using System.IO;
using Microsoft.Win32;
using System.Windows.Forms;

class Uninstaller
{
    [STAThread]
    static void Main()
    {
        string programDir = @"C:\Program Files\Trumpet";
        string userMedia = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".customTrumpets");

        try
        {
            // Kill running Trumpet processes
            foreach (var p in Process.GetProcessesByName("Trumpet"))
            {
                p.Kill();
            }

            // Delete program folder
            if (Directory.Exists(programDir))
            {
                Directory.Delete(programDir, true);
            }

            // Delete user media folder
            if (Directory.Exists(userMedia))
            {
                Directory.Delete(userMedia, true);
            }

            // Remove startup registry entry
            using (RegistryKey runKey = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Run", true))
            {
                runKey?.DeleteValue("Trumpet", false);
            }

            // Remove uninstall registry key
            using (RegistryKey uninstallKey = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Uninstall", true))
            {
                uninstallKey?.DeleteSubKeyTree("Trumpet", false);
            }

            MessageBox.Show("Trumpet has been completely removed.", "Uninstall Complete", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
        catch (Exception ex)
        {
            MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }
}
