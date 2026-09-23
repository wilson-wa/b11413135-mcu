# AI 協作日誌 #2（小組 repo 整理與 Git 協作）

- 姓名：王毓淂
- 學號：B11413135
- 日期：2026/09/23
- 使用的 AI：Claude
- 相關 repo：belongho0117-lgtm/mcu（小組共用 repo，我是協作者）

| 任務 | 我問 AI 什麼 | AI 的回答重點 | 我怎麼驗證 | 結果與收穫 |
|---|---|---|---|---|
| 整理小組 repo 的 README | 組員把 main.c 的程式碼直接貼進 README，要怎麼處理 | README 是專案說明文件，應寫組別、成員、開發板、資料夾結構；程式碼要放在 .c 檔並由專案資料夾管理 | 修改後在 GitHub 網頁開啟 README，確認顯示成標題與清單，不再是一整段程式碼 | 學到 README 與原始碼的用途不同；共用 repo 的檔案要先看清楚內容再改，避免蓋掉別人的東西 |
| 在小組 repo 加入 .gitignore | 小組 repo 也要排除編譯產物嗎、內容要放什麼 | 與個人 repo 相同，排除 Debug/、Release/、*.o、*.elf 等 CubeIDE 編譯產物，只版控原始碼 | 執行 `git status` 確認編譯產物不再被列出；push 後在 GitHub 檔案列表確認 .gitignore 存在 | 避免把幾十 MB 的編譯產物推上去；也學到 .gitignore 只對「尚未被追蹤」的檔案有效 |
| 分次 commit | 一次改了 README 和 .gitignore，要一起 commit 還是分開 | 依 Git 教學投影片「一個修改一個 commit」，分成兩次，訊息要能看出改了什麼 | 對照 Git 教學投影片；push 後在 GitHub 的 Commits 列表確認是兩筆獨立紀錄 | commit 訊息依課程規定加註「（AI 輔助）」；學到 commit 切細之後，日後要回溯或還原比較容易 |
| push 前先同步 | 共用 repo 直接 push 會不會蓋到組員的修改 | push 前先 `git pull`，把遠端的更新併進本機，再 push | 實際執行 `git pull` 後再 `git push`，成功顯示 `main -> main` | 學到多人共用 repo 的基本流程是 pull → 修改 → commit → pull → push |
| 排查 Contributors 沒有出現我 | push 成功了，但 GitHub 右邊的 Contributors 沒有我的帳號 | GitHub 是用 commit 內的 email 對應帳號，不是用登入的帳號；可用 `git log` 查作者信箱，再確認該信箱有加入 GitHub 帳號並完成驗證 | 執行 `git log -5 --format="%h %an <%ae> %s"` 確認作者為王毓淂／學校信箱；到 GitHub Settings → Emails 確認學校信箱狀態為 Verified；再開 Commits 列表確認顯示 wilson-wa 與我的頭像 | 確認身分設定正確，Contributors 欄位只是統計有快取延遲。學到「commit 的作者」與「push 的帳號」是兩件事 |
| 確認 Verified 標章的意義 | 為什麼組員的 commit 有綠色 Verified，我的沒有 | Verified 代表該 commit 有數位簽章，在 GitHub 網頁上直接編輯會自動簽章；用本機 Git 指令 commit 若未設定 GPG／SSH 簽章就不會有此標章 | 對照組員兩筆網頁編輯的 commit 與我兩筆本機 commit 的差異 | 確認這與作者歸屬、Contributors 統計無關，本次作業不需額外設定簽章 |

## 本週總結

本次協助整理小組共用 repo：把誤貼成程式碼的 README 改寫為小組說明、加入 .gitignore 排除編譯產物，並依課程規定分成兩個 commit 後 push 成功。

過程中最大的卡點是 push 成功後 Contributors 仍沒有出現我的帳號。一開始以為是 push 失敗或身分設錯，實際用 `git log` 檢查後才確認 commit 的作者資訊正確，問題只在於「GitHub 用 commit 的 email 對應帳號」這個機制，以及統計欄位本身有更新延遲。學到遇到問題時要先用指令確認本機的實際狀態，而不是只看網頁畫面猜測。

AI 給的建議我都有實際驗證：指令部分用 `git status`、`git log` 確認，結果部分一律回到 GitHub 網頁檢查。其中 AI 曾提出「若 commit 被記成組員身分，可改寫歷史修正」的方向，但考量這是小組共用 repo，`git push --force` 會影響組員，最後未採用，改以確認信箱對應的方式處理。
