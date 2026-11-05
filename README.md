Medicine Reminder & Expiry Tracker

Project Overview
The Medicine Reminder & Expiry Tracker is a console-based application developed using the C programming language. It helps users manage medicine records, track expiry dates, monitor stock levels, and maintain medicine-related information.
The project provides two separate modes:
1. Consumer Mode
2. Pharmacist Mode
Each mode maintains its own data file to ensure that consumer and pharmacist records remain separate.

---
Modes of Operation
1. Consumer Mode
This mode is designed for individuals who want to manage their personal medicines.
Features:
- Add medicines with quantity and expiry date.
- Set minimum stock warning levels.
- Set the number of doses required per day.
- Record the first dose time.
- View and search medicine records.
- Update or delete medicine details.
- Record doses taken.
- View today's medicine reminders.
- Generate expiry and low-stock reports.

2. Pharmacist Mode
This mode is designed for basic pharmacy inventory management.
Features:
- Add medicines with quantity and expiry date.
- Maintain batch numbers and supplier details.
- Store price per unit.
- Set minimum stock/reorder levels.
- View and search medicine records.
- Update or delete medicine details.
- Dispense medicine and automatically update stock.
- Generate expiry and low-stock reports.

---
Key Features
- Medicine Management: Add, view, search, update and delete medicine records.
- Expiry Tracking: Identify expired medicines and medicines expiring within the next 30 days.
- Low Stock Alerts: Identify medicines whose quantity reaches or falls below the minimum stock level.
- Dose Tracking: Record daily doses taken in Consumer Mode.
- Daily Reminders: Display medicines with pending doses.
- Stock Dispensing: Automatically reduce medicine quantity in Pharmacist Mode.
- Sorting: Arrange medicines according to their expiry dates.
- Input Validation: Validate dates, times, quantities and menu choices.
- Data Persistence: Save records in binary files so that information remains available after restarting the program.
- Separate Data Storage: Maintain independent records for both modes.

---
Technologies Used
- Programming Language: C
- Compiler: GCC
- Libraries:
  - stdio.h
  - stdlib.h
  - string.h
  - ctype.h
  - time.h

C Programming Concepts Used
- Structures
- Functions
- Pointers
- Dynamic Memory Allocation
- File Handling
- Arrays and Strings
- Searching and Sorting
- Date and Time Handling
- Input Validation

---
How to Compile and Run
Requirements
- GCC Compiler
- Command Prompt or Terminal

Step 1: Compile the Program
Save the source code as project.c.
For Windows: gcc project.c -o project.exe
For Linux/macOS: gcc project.c -o project

Step 2: Run the Program
For Windows: .\project.exe
For Linux/macOS: ./project

Step 3: Select the Mode
After starting the program:
1. Select 1 for Consumer Mode.
2. Select 2 for Pharmacist Mode.
3. Follow the menu instructions.
4. Select 0 to save and exit.

---
Data Storage
The project uses binary file handling to store medicine records.
|      Mode       |      Data File      |
|-----------------|---------------------|
| Consumer Mode   | consumer_data.dat   |
| Pharmacist Mode | pharmacist_data.dat |

The program automatically saves records and loads previously stored information whenever the corresponding mode is started.

---
Project Screenshots
1. Program Startup and Mode Selection
<img width="429" height="208" alt="image" src="https://github.com/user-attachments/assets/d053928f-6fc7-4a76-9d31-123b4fa8ff79" />

2. Consumer Mode Menu
<img width="533" height="426" alt="image" src="https://github.com/user-attachments/assets/dadbf8dc-c807-4017-8b82-095978066d87" />


3. Adding a Medicine
<img width="541" height="486" alt="image" src="https://github.com/user-attachments/assets/e7e849b3-801e-478d-bbc0-0cfc7eaef441" />


4. Viewing and Searching Medicines
<img width="534" height="484" alt="image" src="https://github.com/user-attachments/assets/3ea1160f-480a-4a94-99ea-e44054005e4e" />
<img width="518" height="501" alt="image" src="https://github.com/user-attachments/assets/3da50611-3c8c-429b-bca9-69b946c73e61" />


5. Today's Medicine Reminders
<img width="528" height="503" alt="image" src="https://github.com/user-attachments/assets/ea805374-69ba-4ce2-b166-034fb2b0b2fc" />


6. Expiry Report
<img width="534" height="506" alt="image" src="https://github.com/user-attachments/assets/3dac71e5-02fb-489d-b780-dd6d137a8211" />


7. Low Stock Report
<img width="491" height="257" alt="image" src="https://github.com/user-attachments/assets/3c2efb9a-37b4-4e30-a58b-17274b732c37" />


8. Recording a Dose
<img width="526" height="319" alt="image" src="https://github.com/user-attachments/assets/e9b53750-07b2-4280-8fc1-16af995c4780" />


9. Pharmacist Mode Menu
<img width="497" height="408" alt="image" src="https://github.com/user-attachments/assets/503c5304-8e52-42ab-ae16-e0c9bdbfc62d" />


10. Dispensing Medicine Stock
<img width="541" height="347" alt="image" src="https://github.com/user-attachments/assets/ae8e4fae-670e-45a5-b56b-1635f5cd2071" />


11. Sorting Medicines by Expiry Date
<img width="541" height="320" alt="image" src="https://github.com/user-attachments/assets/92505681-ea8f-43f5-a668-c7181fe81504" />


12. Saving Data and Exiting
<img width="521" height="247" alt="image" src="https://github.com/user-attachments/assets/aed4da34-06cb-4da2-89fa-3c93ed01b138" />


---
Limitations
- The application is completely console-based and does not have a graphical user interface.
- Reminders are displayed when the user selects the reminders option; the program does not send background notifications.
- Consumer Mode records dose information but does not verify prescriptions or provide medical advice.
- Data is stored locally in binary files and is not synchronised across devices.

---
Conclusion
The Medicine Reminder & Expiry Tracker demonstrates the practical implementation of C programming concepts to solve a real-world problem.
It provides an organised way to manage personal medicines and pharmacy inventory while reducing the chances of overlooking expiry dates, low stock levels and daily dose records.
The project combines file handling, structures, pointers, dynamic memory allocation and various other programming concepts into a single practical application.
