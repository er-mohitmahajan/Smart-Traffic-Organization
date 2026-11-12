Option Explicit

Dim fso, shell, projectDir, appPath, raylibDir, packagesDir
Dim packageFolder, compilerBin, candidateBin, processEnv, currentPath

Set fso = CreateObject("Scripting.FileSystemObject")
Set shell = CreateObject("WScript.Shell")

projectDir = fso.GetParentFolderName(WScript.ScriptFullName)
appPath = fso.BuildPath(projectDir, "build-current-x64\smart_traffic.exe")
raylibDir = fso.BuildPath(projectDir, "raylib\lib")

If Not fso.FileExists(appPath) Then
    MsgBox "The application was not found:" & vbCrLf & appPath & vbCrLf & _
           "Build the project first.", vbCritical, "Smart Traffic"
    WScript.Quit 1
End If

If Not fso.FileExists(fso.BuildPath(raylibDir, "raylib.dll")) Then
    MsgBox "Raylib was not found:" & vbCrLf & _
           fso.BuildPath(raylibDir, "raylib.dll"), vbCritical, "Smart Traffic"
    WScript.Quit 1
End If

packagesDir = fso.BuildPath(shell.ExpandEnvironmentStrings("%LOCALAPPDATA%"), _
                            "Microsoft\WinGet\Packages")
compilerBin = ""

If fso.FolderExists(packagesDir) Then
    For Each packageFolder In fso.GetFolder(packagesDir).SubFolders
        If InStr(1, packageFolder.Name, _
                 "BrechtSanders.WinLibs.POSIX.UCRT_", vbTextCompare) = 1 Then
            candidateBin = fso.BuildPath(packageFolder.Path, "mingw64\bin")
            If fso.FileExists(fso.BuildPath(candidateBin, "g++.exe")) Then
                compilerBin = candidateBin
                Exit For
            End If
        End If
    Next
End If

If compilerBin = "" Then
    MsgBox "The MinGW runtime folder was not found under:" & vbCrLf & _
           packagesDir, vbCritical, "Smart Traffic"
    WScript.Quit 1
End If

Set processEnv = shell.Environment("Process")
currentPath = processEnv("Path")
processEnv("Path") = compilerBin & ";" & raylibDir & ";" & currentPath
shell.CurrentDirectory = projectDir
shell.Run """" & appPath & """", 1, False
