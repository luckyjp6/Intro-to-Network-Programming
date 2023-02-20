# Intro to Network programming

## Lab 1 -- Find Magic Packet
- 目標：熟悉docker和wireshark/tcpdump的運作，並瞭解如何錄製封包、解析封包。
- 實作內容：和老師的server互動，截錄封包並找出最特別的一個。

## Lab 2 -- Unpacker
- 目標：如何解析自訂義binary結構，了解__attribute__((packed))的運用。
- 實作內容：嘗試從一個自訂義的binary資料結構中解析出多個檔案，其內容包括檔名、檔案大小和checksum等細節，並分別存成單一的檔案，最後會由老師提供的checker執行檔檢查解析的正確性。

## Lab 3
### Part 1 - Count Words
- 目標：了解如何建立TCP聯線（了解socket的使用）及如何讀取/傳送資料到網路上，並知悉實作上的細節。
- 實作內容：連上老師的server之後，server會丟給我們肉眼難以計數的亂碼，我們要寫一個程式計算接收到多少字元並回傳，server會驗證答案是否正確。
### Part 2 - Traffic Shaper
- 目標：了解如何控制網路傳輸速度，和其實作上的細節。
- 實作內容：具體希望的傳輸速度會做為執行檔的參數傳入，程式將考慮TCP/IP header及payload，控制傳輸速度，我使用的方法是先將一秒內需要傳輸的資料量傳完，再計算傳輸完資料後，這一秒還剩餘時間並讓程式休眠。

## Lab 4 - Multi-client Server
- 目標：了解如何架設能夠同時服務多個client的server，並學習使用fork和dup。
- 實作內容：透過fork出children處理新連線的方式同時服務多個client，主程式只負責接收連線，並處理terminate的children。

## Lab 5 - Chat Room
- 目標：練習如何照規定標準實作聊天室相關的應用並了解廣播訊息相關實作細節。
- 實作內容：依照老師給出的標準實作簡易版的聊天室（完整版、照RFC1459標準實作的聊天室在Hw1），並處理以特定格式印出時間、廣播訊息，及client意外斷線等情形。
- 聊天室的具體指令如：
    - "/name"：更換暱稱。
    - "/who" ：列出聊天室中所有使用者（暱稱、ip address、port）。

## Lab 6 - Command/Data Channel
- 目標：練習command和data由兩個不同channel傳輸的protocol要如何實作。
- 實作內容：當client分別連上command和data channel之後，server開始計時，並於client傳送/report指令時匯報這段時間內client藉由data channel傳送資料的平均速率是多少，client也可以下/reset指令重置計時器。
- Command channel具體指令如：
    - "/reset"  ：重設計時器
    - "/report" ：回傳資料傳輸平均速率
    - "/clients"：列出現有client數量

## Lab 7 - CTF
這個lab是有關Reentraint問題的CTF競賽：
### 第一題 
有oracle1, oracle2可以互動，secret的產生是隨機的，需要猜對seret才能獲取flag。oracle 1和2的secrete有reentrant的問題，會共用secrete，嘗試答案錯誤三次後，可以拿到其中一隻的secrete，再交給另外一隻即可獲取flag。
### 第二題
設定同第一題，但這題oracle 1和2的random seed有reentrant的問題，會共用seed，嘗試答案錯誤三次後，可以在拿到其中一隻的seed，根據程式邏輯，用該seed跑5次rand()之後，使用server generate secret的code去產生secret即可獲取flag。
### 第三題
可透過server連線至各個網站，連線至localhost即可獲取flag，但server禁止client連線至localhost。server使用gethostbyname()判斷client request的ip address是否為localhost，而gethostbyname有reentrant的問題，且server程式分別使用兩個thread跑worker，worker協助建立連線，會在function do_work裡循環，可以卡thread切換時gethostbyname()使用全域變數儲存回傳結果造成的bug。首先用server的request指令建兩條連線（port都是10000），其中一條連線的host設置成localhost，另一條設成任一localhost以外的連線（如google.com），在某個瞬間剛好google.com通過了localhost條件判斷，且在連上網站之前，剛好切換成另一個worker，另一個worker在上一次竊換thread前剛好執行過gethostbyname，種種巧合下（需等待一些時間）就能繞過localhost條件判斷和localhost連線並獲取flag。

## Lab 8 - Robust UDP Challenge
- 目標：client嘗試在網路很糟糕的環境中傳輸封包且保證server端收到的資料正確（使用UDP），課堂中所有同學將參與排名，排名依據為檔案傳送完成度和傳送花費時間（以client結束為時間終止）。
- 實作內容：自行設計client及server，兩者皆於argv中獲取待傳輸資料的位置或收到的資料應存放的位置，當程式結束會有檢查對應位置檔案的正確性，共將傳輸1000個檔案，檔案大小1000kb~32000kb不等，各封包大小限制為1.5kb。
- 我的做法：（最終排名第5）
    1. 標籤機制：額外定義了一個的header，標示封包內資料是第幾個檔案的第幾個fragment。
    2. 溝通機制：server將持續傳送目前尚未收到的檔案/fragment編號給client，client依據收到的訊息傳送對應資料給server。
    3. 終止機制：當server收到所有檔案便會持續傳送大小為1的資料，client收到後便會自行結束。

## Lab 9 - Online Sudoku
- 目標：以Sudoku為媒介，練習實作和server實時互動的client端。
- 實作內容：以TCP連線，透過server給定的指令進行互動，獲取題目和輸出答案。

## Lab 10 - Revisiting Lab 8 via Broadcast and Raw Socket
- 目標：以broadcast的方式，運用raw socket重現Lab 8（需自行填寫ip header），但這次網路環境較優良。
- 實作內容：架構上和Lab 8類似，但須以broadcast的方式收發封包，因為網路環境良好，server不再傳送request給client，實作的難點變為如何填寫ip header和如何控制傳輸速度以保證server的receiving buffer不會因為沒有空位而丟棄封包。
- 實作心得：這次用更加物件導向的方式撰寫程式，並活用Lab 2學到的__attribute__((packed))來處理封包，有集整門課之大成的感覺，模組化的寫法十分令人快樂，不知不覺間，自己也成長了，寫的程式終於有些像樣。