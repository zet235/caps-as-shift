# CapsAsShift

Windows 常駐程式：將**實體 Caps Lock 當作左 Shift**。執行時 Caps Lock 不再鎖定大小寫；長按時可搭配字母使用。單按是否切換輸入法，取決於目前輸入法的「單按 Shift」設定。

## 下載

到 [GitHub Releases](https://github.com/zet235/no-capslock/releases) 下載 Windows x64 的 **CapsAsShift.exe**，直接執行即可使用。

## 編譯與使用

需要 Windows 和 MinGW-w64 `g++`（支援 C++17）。在本資料夾執行：

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

會執行測試並產生 `CapsAsShift.exe`，直接啟動該檔即可使用。程式介面使用英文；右鍵點擊系統匣圖示，可勾選 **Start with Windows** 或選 **Exit**。自啟設定只寫入目前使用者的 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`；退出不會關閉下次登入的自啟。移動 EXE 後重新勾選自啟即可更新路徑。不會安裝驅動程式，也無需 PowerToys。

重新編譯前，請先從系統匣選 **Exit**。啟動程式前也請先關閉 Caps Lock 大寫鎖定；程式會保留原本的大小寫鎖定狀態。如果已經開啟，請先退出程式、按 Caps Lock 關閉鎖定，再重新啟動。

## 圖示

EXE 和系統匣使用深藍鍵帽搭配白色 Shift 箭頭。`assets/caps-as-shift.ico` 包含 16–256 px 的九種尺寸；`assets/caps-as-shift.png` 是預覽圖。圖示已嵌入 EXE，執行時不需另外攜帶圖片檔。

圖示來源是 `tools/generate-icon.ps1`，使用 Windows 內建的 .NET 繪圖產生，不需下載素材或安裝繪圖套件。完整建置會自動產生圖示並透過 MinGW 的 `windres` 嵌入 EXE。

## 實機驗證

自動測試涵蓋按鍵聯集、重複按下、重疊放開、注入事件過濾、自啟登錄檔、單一執行個體、系統匣圖示及退出。實體鍵盤與輸入法仍需在你使用的 Windows 桌面確認：

1. 先在文字編輯器試按**實體左 Shift**能否單按切換中英，再試單按 Caps Lock，比較行為。
2. 試 Caps Lock + 字母、長按 Caps Lock、右 Shift，以及 Caps Lock／左 Shift 依兩種順序按下再放開。檢查沒有提早釋放或卡住 Shift。
3. 重複開啟 EXE，確認只有一個系統匣圖示；從圖示退出，確認鍵位恢復。重新啟動 Windows Explorer 後，確認圖示恢復。
4. 手動勾選／取消登入自啟並檢查效果；需要驗證重新登入時，請由本人在互動桌面操作。

未提升權限的 EXE 在系統提升權限的視窗上可能無法攔鍵或送鍵。強制終止 EXE 不保證注入的 Shift 會收到 key-up；請用系統匣的 **Exit**。

### 高 CPU 負載或系統卡頓

高 CPU 使用率不代表一定會漏鍵，但如果鍵盤 hook 無法及時回應，Windows 可能略過映射，並在逾時後靜默移除 hook；此時系統匣圖示仍可能存在。Windows 10 1709 之後允許的 hook 逾時上限為 1000 毫秒，實際值取決於 `LowLevelHooksTimeout` 設定，並非 CPU 百分比的門檻。

目前 hook 和系統匣共用執行緒，尚未完成高負載下的實體鍵盤／輸入法壓力測試。如果 Caps Lock 映射停止，可從 **Exit** 退出後重新啟動程式。詳見 [Microsoft 的 LowLevelKeyboardProc 文件](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc)。
