# usermode-taskscheduler, @8snd
This is a POC of a usermode taskscheduler for roblox, 
> ⚠ **Disclaimer**  
> This code is **detected** (`ReadProcessMemory`) without the use of a driver. We do **not** accept any liability for how this code is used.  
> We are **not affiliated** with Roblox, and we do **not** condone or support cheating in any form. This project is intended strictly for **development and research purposes**.  

## Notes  
- The offsets in `task.hpp` **will need updating**.  
- This project includes an **ImGui overlay** displaying relevant task scheduler information:  


```
Roblox Task Scheduler - @8snd
Process Handle: 0xFFFFFFFFFFFFFFF
Base Address: 0xFFFFFFFFFFF
Task Scheduler: 0xFFFFFFFFFF
DataModel: 0xFFFFFFFFFFF
RenderJob: 0xFFFFFFFFFFF
Total Jobs: 00
In game, DataModel is Ugc. (or, In homepage, DataModel is LuaApp)
```
