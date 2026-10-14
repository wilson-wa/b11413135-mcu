# 期中專題：智慧排隊叫號機

微算機原理及應用　期中個人專題｜B11413135 王毓淂｜NUCLEO-G474RE（STM32G474RE）

客人按按鈕抽號、店員在電腦打指令叫號、板子用燈號顯示狀態。排隊名單本身就是一個環形緩衝。

## 目錄

- [功能](#功能)
- [硬體接線](#硬體接線)
- [操作方式](#操作方式)
- [燈號說明](#燈號說明)
- [程式架構](#程式架構)
- [對應課程章節](#對應課程章節)
- [專案目錄結構](#專案目錄結構)
- [開發進度](#開發進度)

## 功能

| 角色 | 功能 |
|---|---|
| 客人 | 按 PC9 按鈕抽號，電腦顯示「抽到幾號、前面還有幾人」 |
| 店員 | 在電腦終端機打 `next`／`ok`／`list`／`skip` |
| 自動 | 叫號後 10 秒內沒有 `ok`，自動過號並叫下一位 |
| 緊急 | 按板子上的藍色按鈕（PC13）暫停服務，再按一次恢復 |
| 燈號 | 黃燈亮度＝排隊人數；綠燈叫號中呼吸、客人到了恆亮；紅燈＝緊急 |

## 硬體接線

| 元件 | 腳位 | 設定 | 接法 |
|---|---|---|---|
| 綠 LED | PC8 | TIM3_CH3（PWM） | PC8 → 電阻 → LED 長腳，短腳 → GND |
| 黃 LED | PC7（Arduino D9） | TIM3_CH2（PWM） | PC7 → 電阻 → LED 長腳，短腳 → GND |
| 紅 LED | PC6 | GPIO_Output | PC6 → 電阻 → LED 長腳，短腳 → GND |
| 抽號按鈕 | PC9 | EXTI9 下降緣＋內部上拉 | 按鈕一腳接 PC9，對角腳接 GND |
| 緊急按鈕 | PC13 | EXTI13 上升緣 | 板子上的藍色 B1，不用另外接 |
| UART | PA2／PA3 | LPUART1 115200 8N1 | 透過 ST-LINK USB（虛擬 COM 埠） |

> 黃燈原本規劃在 PC5，但 PC5 沒有 Timer 通道不能輸出 PWM，所以改到 PC7（TIM3_CH2）。

TIM3：Prescaler 84、Counter Period 19999 → 170 MHz ÷ 85 ÷ 20000 = **100 Hz**（不閃爍）

## 操作方式

1. 燒錄後，用 PuTTY 開啟 ST-LINK 的 COM 埠：Serial、**115200**
2. 按板子 Reset，終端機顯示歡迎訊息
3. 按 PC9 抽號，在終端機打指令（小寫，按 Enter 送出）

| 指令 | 作用 | 範例回覆 |
|---|---|---|
| `next` | 叫下一位，開始 10 秒倒數 | `Now serving #1 (2 waiting).` |
| `ok` | 客人到了，停止倒數 | `#1 arrived.` |
| `list` | 列出目前叫號和排隊名單 | `Serving: #1 (calling) \| Waiting: #2 #3` |
| `skip` | 過號，直接叫下一位 | `#1 skipped.` |

緊急暫停時只能用 `list`，其他指令和抽號都會被擋下。

## 燈號說明

| 狀態 | 綠燈（PC8） | 黃燈（PC7） | 紅燈（PC6） |
|---|---|---|---|
| 沒人排隊 | 滅 | 滅 | 滅 |
| 有人排隊 | — | 亮度隨人數增加（10 人最亮） | 滅 |
| 叫號中（等客人） | 呼吸 | — | 滅 |
| 客人已到（`ok`） | 恆亮 | — | 滅 |
| 緊急暫停 | 滅 | 滅 | 恆亮 |

## 程式架構

### 中斷只登記，主迴圈做事

| 中斷 | 優先權 | 中斷裡只做 | 主迴圈處理 |
|---|---|---|---|
| EXTI15_10（PC13 緊急） | **0（最高）** | `emergencyFlag = 1` | 去彈跳 → 暫停／恢復 |
| LPUART1（店員指令） | 1 | 收到的字放進 `rxRing` | 組成一行 → 執行指令 |
| EXTI9_5（PC9 抽號） | 2（最低） | `ticketFlag = 1` | 去彈跳 → 抽號 |

### 兩個環形緩衝

| 緩衝 | 存什麼 | 放進去（head） | 拿出來（tail） | 大小 |
|---|---|---|---|---|
| `queue` | 排隊的號碼 | 客人按 PC9 抽號 | 店員 `next` 叫號 | 11 格（犧牲一格，最多 10 人） |
| `rxRing` | 店員打的字 | UART 中斷 | 主迴圈 | 64 格 |

判斷規則：`head == tail` 是空的；`(head + 1) % N == tail` 是滿的。

### 非阻塞計時（不用 HAL_Delay 等待）

| 功能 | 做法 |
|---|---|
| 按鈕去彈跳 | 距離上次超過 300 ms，且按鈕現在真的是按下狀態才算數 |
| 10 秒自動過號 | `HAL_GetTick() - callStartTick >= 10000` |
| 綠燈呼吸 | 每 10 ms 改一次 CCR（平方曲線） |

## 對應課程章節

| 章節 | 用在哪裡 |
|---|---|
| W3 GPIO | 紅燈輸出、按鈕輸入與上拉電阻 |
| W4 時脈 | 170 MHz 系統時脈、HAL_GetTick 1 ms 計時、PWM 頻率計算 |
| W5 中斷與 EXTI | PC9／PC13 外部中斷、NVIC 優先權、ISR 只設旗標、去彈跳 |
| W6 UART 與環形緩衝 | LPUART1 收發、中斷收字、指令解析、兩個環形緩衝 |
| W7 Timer／PWM | TIM3 兩路 PWM 控制亮度、呼吸燈 |

## 專案目錄結構

```
midterm_queue/
├── Core/                              # 專案核心程式碼
│   ├── Inc/                           # 標頭檔目錄
│   │   ├── main.h                     # 腳位別名（TICKET_BTN、STOP_BTN）與函式原型
│   │   ├── stm32g4xx_hal_conf.h       # HAL 模組開關（GPIO、UART、TIM）
│   │   └── stm32g4xx_it.h             # 中斷服務函式標頭檔
│   ├── Src/                           # C 語言原始碼目錄
│   │   ├── main.c                     # 主程式：抽號、叫號、指令解析、環形緩衝、PWM 燈號、中斷 Callback
│   │   ├── stm32g4xx_hal_msp.c        # 底層初始化（LPUART1 PA2／PA3、TIM3 PC7／PC8 AF2）
│   │   ├── stm32g4xx_it.c             # 中斷入口（EXTI9_5、EXTI15_10、LPUART1）
│   │   ├── syscalls.c                 # 系統呼叫（CubeIDE 自動產生）
│   │   ├── sysmem.c                   # 記憶體配置（CubeIDE 自動產生）
│   │   └── system_stm32g4xx.c         # 系統時脈與 CMSIS 底層啟動配置
│   └── Startup/
│       └── startup_stm32g474retx.s    # 組合語言啟動檔（向量表、Reset_Handler）
├── Drivers/                           # 硬體驅動庫
│   ├── CMSIS/                         # ARM Cortex-M 核心標準抽象層
│   └── STM32G4xx_HAL_Driver/          # ST 官方 HAL 驅動
├── midterm_queue.ioc                  # STM32CubeMX 周邊組態設定檔
├── STM32G474RETX_FLASH.ld             # Linker Script（Flash 執行）
├── STM32G474RETX_RAM.ld               # Linker Script（RAM 執行）
└── README.md                          # 專案說明文件
```

## 開發進度

| 關卡 | 內容 | 狀態 |
|---|---|---|
| 0 | CubeMX 建專案、中斷優先權 | 完成 |
| 1 | 開機訊息、三色燈自我測試 | 完成 |
| 2 | PC9 中斷抽號、排隊環形緩衝 | 完成 |
| 3 | UART 指令 next／list／skip | 完成 |
| 4 | ok 指令、10 秒自動過號 | 完成 |
| 5 | PC13 緊急暫停 | 完成 |
| 6 | TIM3 PWM：黃燈亮度＝人數、綠燈呼吸 | 完成 |
| 7 | 輸入捕捉量測 PWM、`measure`／`stats` 指令 | W7 上課後進行 |
