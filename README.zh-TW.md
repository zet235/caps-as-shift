<div align="center">
  <img src="assets/caps-as-shift.png" width="128" height="128" alt="CapsAsShift：深藍鍵帽搭配白色 Shift 箭頭">
  <h1>CapsAsShift</h1>
  <p><strong>讓 Caps Lock，成為另一顆左 Shift。</strong></p>
  <p>輕量 Windows 系統匣工具，單一 EXE，可選擇登入後自動啟動。</p>
  <a href="https://github.com/zet235/no-capslock/actions/workflows/build.yml"><img src="https://github.com/zet235/no-capslock/actions/workflows/build.yml/badge.svg?branch=main" alt="Windows 建置狀態"></a>
  <a href="https://github.com/zet235/no-capslock/releases/latest"><img src="https://img.shields.io/github/v/release/zet235/no-capslock?color=315b96" alt="最新版本"></a>
  <img src="https://img.shields.io/badge/platform-Windows%20x64-0078d6" alt="平台：Windows x64">
  <p><a href="https://github.com/zet235/no-capslock/releases/latest/download/CapsAsShift.exe"><strong>下載 Windows x64 版</strong></a> · <a href="https://github.com/zet235/no-capslock/releases">所有版本</a></p>
</div>

<p align="center"><a href="README.md">English</a> · <strong>繁體中文</strong></p>

---

## 為什麼使用 CapsAsShift？

如果你的輸入法使用單按 Shift 切換中英文，CapsAsShift 就能讓位置方便的 Caps Lock 也派上用場。程式執行時，實體 Caps Lock 會映射成左 Shift，長按搭配其他按鍵也可以使用。

| 功能 | 使用方式 |
| --- | --- |
| **Caps Lock → 左 Shift** | 單按等同 Shift，長按可搭配字母或其他按鍵。 |
| **處理兩鍵重疊** | 同時追蹤 Caps Lock 和實體左 Shift，最後一顆放開時才釋放 Shift。 |
| **系統匣控制** | 英文選單提供 **Start with Windows** 與 **Exit**。 |
| **單一 EXE** | 圖示與 MinGW 執行期已包含在程式內，免安裝、不需 PowerToys。 |
| **GitHub 雲端編譯** | 從 **v0.1.1** 起，Release 的 EXE 由 GitHub Actions 編譯並測試。 |

> **輸入法行為：** 只有目前輸入法設定為「按 Shift 切換語言」時，單按才會切換中英文。CapsAsShift 不會更改輸入法設定。

## 快速開始

1. [下載 **CapsAsShift.exe**](https://github.com/zet235/no-capslock/releases/latest/download/CapsAsShift.exe)，放在你打算保留的位置。
2. 先關閉 Caps Lock 原本的大小寫鎖定，再執行 EXE。
3. 在系統匣找到藍色 Shift 圖示；它可能收在隱藏圖示選單中。

試著按住 <kbd>Caps Lock</kbd> 搭配字母，再比較單按 Caps Lock 與平常用 <kbd>Shift</kbd> 切換輸入法的行為。

### 系統匣選單

| 選項 | 功能 |
| --- | --- |
| **Start with Windows** | 開啟或關閉目前 Windows 帳號登入後自動啟動。 |
| **Exit** | 停止按鍵映射並結束程式。 |

同一時間只會執行一份程式。退出不會關閉下次登入的自啟設定。如果移動 EXE，請從新位置執行，再勾選 **Start with Windows** 更新路徑。

自啟設定存放在 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` 的 `CapsAsShift` 值。

## 由 GitHub 編譯

[Windows 建置流程](.github/workflows/build.yml) 使用 GitHub 提供的 Windows 主機與 MinGW-w64 UCRT64 工具鏈。

| 觸發方式 | 執行內容 |
| --- | --- |
| 推送到 `main` 或建立 Pull Request | 編譯、執行四組 C++ 測試、產生圖示，再上傳 Windows 建置產物。 |
| 在 Actions 頁面選 **Run workflow** | 手動產生可下載的建置產物。 |
| 推送 `v*` 版本標籤 | 執行相同建置，並將 EXE 與 `SHA256SUMS.txt` 發布到 GitHub Releases。 |

發布工作會下載**同一次 workflow 執行**產生的檔案，確認 SHA-256 校驗值後才上傳，不會拿開發者電腦的 EXE 發布。

雲端建置使用 `-SkipGuiSmoke`：CI 會執行按鍵狀態、事件分派、登錄檔與生命週期測試；互動式系統匣和實體鍵盤／輸入法則需在 Windows 桌面驗證。

<details>
<summary><strong>驗證下載檔案</strong></summary>

下載同一個 Release 的 `SHA256SUMS.txt`，再與下列指令的結果比對：

```powershell
Get-FileHash .\CapsAsShift.exe -Algorithm SHA256
```

</details>

## 從原始碼建置

開發環境需要 Windows、PowerShell 5.1，以及已加入 `PATH` 的 MinGW-w64 `g++`／`windres`。程式使用 C++17。

重新編譯前，請先退出正在執行的 CapsAsShift：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

腳本會執行四組 C++ 測試、產生 `CapsAsShift.exe`，再測試系統匣、啟動與退出。在非互動式環境可使用：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -SkipGuiSmoke
```

## 運作方式

Win32 `WH_KEYBOARD_LL` hook 攔截實體 Caps Lock 與左 Shift；狀態機合併兩鍵的按住狀態，再由 `SendInput` 送出左 Shift 的按下／放開。程式自己產生的事件有標記，會直接放行以避免循環。程式不會記錄按鍵歷史或使用網路。

藍色鍵帽圖示由 [`tools/generate-icon.ps1`](tools/generate-icon.ps1) 使用 Windows 內建的 .NET 繪圖產生。[ICO](assets/caps-as-shift.ico) 包含 16–256 px 的九種尺寸，[PNG](assets/caps-as-shift.png) 是 512 px 預覽。系統匣和 EXE 圖示都使用嵌入的 Windows 資源。

## 使用限制與桌面驗證

- **既有大小寫鎖定：** 程式會保留啟動前的 Caps Lock 狀態。如果本來已開啟，請先選 **Exit**、關閉 Caps Lock，再重新執行。
- **系統管理員視窗：** 以一般權限啟動的程式，可能無法在較高權限的視窗內映射按鍵。
- **高 CPU 負載或卡頓：** CPU 使用率高不代表一定失敗，但 hook 若太晚回應，Windows 可能略過或靜默移除它。映射停止後圖示仍可能存在，可退出並重新執行以重新安裝 hook。
- **正常退出：** 請使用 **Exit**，讓程式有機會清理合成按鍵狀態，避免直接強制終止。

目前 hook 與系統匣共用執行緒。Windows 10 1709 之後允許的 hook 逾時上限是 1,000 毫秒，實際 `LowLevelHooksTimeout` 設定可能更低；尚未完成高負載下的實體輸入壓力測試。詳見 [Microsoft 的 hook 文件](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc)。

請在自己的環境確認輸入法切換、快速按鍵與兩鍵重疊、鎖定／解鎖、睡眠／喚醒、Explorer 重新啟動及登入自啟。已驗證的修正與剩餘限制記錄在 [security 與 code review 報告](docs/reviews/2026-09-24-review.md)。
