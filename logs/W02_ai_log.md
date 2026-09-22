\# AI 協作日誌 #1（W02：建立 git repo）



\- 姓名：王毓淂

\- 學號：B11413135

\- 日期：2026/09/22

\- 使用的 AI：Claude



| 任務 | 我問 AI 什麼 | AI 的回答重點 | 我怎麼驗證 | 結果與收穫 |

|---|---|---|---|---|

| 設定 git 姓名與信箱 | 安裝 git 後要怎麼設定使用者資料 | 使用 `git config --global user.name` 與 `user.email`，並用 `git config --global --list` 檢查 | 對照 Git 教學投影片 p.5；實際執行檢查指令 | 一開始打成 `-- global`（中間多一個空格），出現 `fatal: not in a git directory`；改成 `--global` 後設定成功。學到指令中的空格會改變意思 |

| 在 Git Bash 貼上指令 | Git Bash 為什麼不能複製貼上 | Git Bash 的 `Ctrl+C` 是中斷指令，要用 `Shift+Insert` 或右鍵 Paste | 實際操作測試 | 改用右鍵貼上成功。學到終端機的快捷鍵和一般軟體不同 |

| 在 GitHub 建立 repo | 開 repo 時要怎麼設定、要放什麼 | 選 Private、勾 Add a README file；每次作業一個資料夾 | 對照 Git 教學投影片 p.15；在 GitHub 網頁確認 repo 狀態 | 第一次忘了勾 README，也沒注意到預設是 Public；後來用網頁補上 README，並到 Settings 改成 Private。學到建立後要主動確認設定 |

| clone repo 到電腦 | repo 要 clone 到哪個位置 | 使用 `git clone`，路徑要全英文，建議放在 `C:\\projects\\` | 對照 Git 教學投影片 p.5–6（STM32CubeIDE 不支援中文路徑） | 成功 clone 到 `C:\\projects\\b11413135-mcu`，第一次登入時用瀏覽器授權 GitHub |

| 建立 .gitignore 並第一次 push | .gitignore 要放什麼、怎麼上傳到 GitHub | .gitignore 排除 Debug/、\*.o、\*.elf 等編譯產物；流程為 add → commit → push | 對照 Git 教學投影片 p.7、p.9、p.12；push 後開 GitHub 網頁確認出現 .gitignore 與 2 Commits | 重開 Git Bash 後忘記 `cd` 進 repo，出現 `not a git repository`；進入資料夾後成功 push。出現的 LF/CRLF warning 只是換行格式提示，不影響結果 |



\## 本週總結



本週完成 git 安裝與設定、建立 GitHub private repo、clone 到本機、加入 .gitignore，並完成第一次 commit 與 push，已在 GitHub 網頁確認檔案。



遇到的問題大多是指令打錯或所在資料夾不對，而 git 的錯誤訊息其實都已經指出原因（例如 `not a git repository` 就是不在 repo 裡）。之後遇到錯誤會先看訊息內容、執行 `git status` 確認狀態，再對照投影片或詢問 AI。



AI 的建議我都有對照 Git 教學投影片確認；其中 AI 一開始建議 clone 到桌面、資料夾用 Lab01 命名，對照投影片後改為 `C:\\projects\\` 路徑與 `hw1/` 資料夾命名，以課程規定為準。

