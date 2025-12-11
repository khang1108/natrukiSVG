# NaTruKi OOP Project

This project is a C++ application developed by NaTruKi Team from [Faculty of Information Technology at Vietnam National University - University of Science](fit.hcmus.edu.vn). The application's core is built entirely in C++, emphasizing the use of Object-Oriented Design Patterns. The Graphical User Interface (GUI) is implemented using the Qt5 framework.

## About The Project

Welcome to our **SVG Viewer** project! This application is the result of a dedicated effort by the **NaTruKi Team**, students in the **24CTT6** class.

This project was conceived not just to build a functional tool, but as a deep dive into **software architecture.** The entire application is written in **C++**, with a core philosophy centered around the practical application of **OOP design patterns** to solve real-world problems. The **Graphic User Interface (GUI)** is powered by the cross-platform **Qt6 frameworks**, ensuring a responsive and intuitive user experience.

## Folder Structure

```bash
NaTruKi_App/
├── CMakeLists.txt
├── include/
│   ├── ui/  
│   |── svg/  
├── src/
│   ├── ui/  
|   |   └─  CMakeLists.txt
│   └── svg/  
|       └─  CMakeLists.txt
├── third_party/
│   └── rapidxml/   
└── docs/
```

## How to Install and Run the App

### Prerequisites

- **CMake** (version 3.16 or higher)
- **Qt6** framework
- **C++17** compatible compiler

### Build Instructions

1. **Clone the repository:**

   ```bash
   git clone https://github.com/khang1108/natrukiSVG.git
   cd NaTruKi_OOP_Project
   ```
2. **Run bash:**

   ```bash
   bash ./build_and_run.sh
   ```

## Contributors

- **[Khang, Phuc Nguyen](https://github.com/khang1108) - Leader:** Manage & Test Functions in Project
- **[Nhat, Hoang Nguyen](https://github.com/nh996):** Build the Parser + Scene Graph
- **[Nghia, Trong Hoang](https://github.com/pumpowhat):** Build UI
- **[Cao, Chi Phan](https://github.com/cpgod36):** Class Defining & Design
