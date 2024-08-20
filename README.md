# PTEViewer
A lightweight tool for visualising paging tables on Windows 10/11 x64, featuring an intuitive ImGui interface.

### Features
1. **Process List:** Find and select a target process, with the ability to sort and search by name, PID and CR3.
2. **Paging Table Viewer:** Inspect PML4E, PDPTE, PDE and PTE entries, displaying hardware and software information such as physical address, write protection, supervisor bit, CoW, etc.
3. **Make Memory Resident:** Move memory pages into physical memory by right clicking on a paging entry.

### Screenshots
![Process List](Images/ProcessList.png)
![Paging Tables](Images/PageTables.png)