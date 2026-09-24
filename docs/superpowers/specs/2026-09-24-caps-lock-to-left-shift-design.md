# Caps Lock → 左 Shift 常駐程式設計

## 目的與完成標準

使用者在 Windows 上經常以 Shift 切換輸入法，希望左手邊的 Caps Lock 也能作為左 Shift 使用。產出一個可自行編譯、無第三方執行期依賴的 C++ Win32 常駐 EXE。執行期間，實體 Caps Lock 的按下與放開應等同左 Shift，不再切換大小寫鎖定；原本的左 Shift 仍可正常使用。單按 Caps Lock 是否切換中英文，以使用者當前 Windows 輸入法設定的「單按 Shift」行為為準，須在實機驗證。

使用者已確認：單一 EXE、系統匣「退出」、避免重複啟動、可選的登入後自動啟動；長按與 Caps Lock／實體左 Shift 同時按住時不可提早放開或卡住 Shift。工作目錄目前為空，尚無可沿用的程式碼。

## 選擇與取捨

使用 Win32 `WH_KEYBOARD_LL` 接收按鍵、`SendInput` 送出替代按鍵，與已調查的 PowerToys 重映射做法相近，但只實作本需求。PowerToys 需要額外常駐軟體；登錄檔 Scancode Map 雖不需常駐，卻需要重新開機、不提供系統匣控制，因此不採用。

## 組件與事件流程

- **鍵盤映射**：安裝低階鍵盤 hook，攔下實體 Caps Lock 與實體左 Shift 的 key-down／key-up；以兩個各自的持有狀態計算「任一鍵按住」的聯集。聯集從無到有時以 `SendInput` 發送一次左 Shift key-down；從有到無時發送一次 key-up。重複 key-down 不改變狀態；右 Shift 與其餘鍵照常交給系統。攔下實體左 Shift 並統一發送合成事件，可避免兩鍵重疊時提早釋放 Shift。
- **注入事件辨識**：在合成事件的 `dwExtraInfo` 放專屬標記，hook 遇到自己的事件直接放行，不再修改狀態。非實體的其他程式注入事件維持原樣，避免把外部程式的按鍵誤算為實體鍵。使用掃描碼與必要的 extended-key 旗標辨識左 Shift；Caps Lock 以其鍵碼辨識。
- **常駐與退出**：建立隱藏視窗跑 Win32 訊息迴圈，持有具名 mutex 防止第二份程式啟動。`Shell_NotifyIcon` 提供系統匣圖示與選單「登入後自動啟動」（勾選狀態）、「退出」。收到 Explorer 重新建立工作列的通知時補回圖示；退出時移除 hook、刪除圖示、釋放 mutex，若程式仍認為合成左 Shift 按住則送出 key-up。
- **登入自啟**：只在使用者勾選時，於目前使用者的 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` 寫入指向目前 EXE 絕對路徑的帶引號命令；取消勾選時移除本程式的項目。不修改整機設定，正常退出不會清除已選擇的自啟。

## 失敗處理與邊界

hook 安裝失敗時顯示錯誤並退出，避免顯示一個不工作的常駐圖示。`SendInput` 無法完成時停止攔截並嘗試釋放合成 Shift，明確顯示錯誤；不得靜默吞掉使用者的 Caps Lock 與左 Shift。寫入或移除自啟失敗時顯示錯誤，選單狀態依實際登錄檔值更新。正常登出與退出盡力釋放 Shift；若 EXE 被強制終止，系統對注入按鍵的處理需實機驗證。

低階 hook 與注入的作用範圍受 Windows 權限／完整性層級限制；驗證時同時測試一般視窗及需要提升權限的視窗，不保證無管理員權限的 EXE 能控制後者。此版本不新增設定介面或其他鍵位映射。

## 驗證

用本機 MinGW `g++` 編出 Windows GUI EXE，先驗證建置與啟動，再在實機測：單按 Caps Lock 的輸入法切換、Caps Lock + 字母、Caps Lock 長按、左 Shift 單按、兩鍵以不同順序按下與放開、右 Shift、重複啟動、系統匣退出後鍵位恢復、登入自啟開關及重新登入。將「單按 Shift 能否切換輸入法」與程式結果對照，避免把輸入法設定差異誤判為映射問題。
