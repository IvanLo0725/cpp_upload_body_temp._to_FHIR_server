### 本程式可在 VS code 等可以運行 C/C++ 的環境，直接上傳體溫至 FHIR server 。
需先下載 curl 函式庫，網址：https://curl.se/windows/
<br>
解壓縮後放到 C 槽根目錄。
<br>
將四個檔案放到同一個資料夾裡面，並建立一個 .vscode 資料夾，將 json 腳本放進去。
<br>
按 ctrl+shift+p ，選擇 run task ，先選擇 build and run fhir_upload ，確定編譯成功，再選擇 run fhir_upload ，輸入溫度後 enter ，即可上傳。

#### Gemini 講解：https://gemini.google.com/share/a1b01f7e1b45
