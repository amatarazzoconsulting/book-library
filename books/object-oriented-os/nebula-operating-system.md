# NEBULA Operating System

**By Anthony Matarazzo (c) 2026**

*57-page technical reference, translated to Markdown from `Object Orient OS - Proton Docs.pdf`. Copyright (c) Anthony Matarazzo 2026. Released under the MIT License.*

## License

```text
MIT License

Copyright (c) 2026 Anthony Matarazzo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## Table of Contents

- [1. Introduction to NEBULA](#1-introduction-to-nebula)
  - [1.1 What is NEBULA?](#11-what-is-nebula)
  - [1.2 Design Philosophy](#12-design-philosophy)
  - [1.3 Key Features](#13-key-features)
- [2. System Architecture Overview](#2-system-architecture-overview)
  - [2.1 Layered Architecture](#21-layered-architecture)
  - [2.2 Component Interaction](#22-component-interaction)
- [NEBULA Operating System](#nebula-operating-system)
- [Complete Kernel Object Directory](#complete-kernel-object-directory)
- [1. KERNEL OBJECT HIERARCHY OVERVIEW](#1-kernel-object-hierarchy-overview)
- [2. KERNEL CORE OBJECTS](#2-kernel-core-objects)
- [3. TASK MANAGEMENT OBJECTS](#3-task-management-objects)
- [4. MEMORY MANAGEMENT OBJECTS](#4-memory-management-objects)
- [5. FILE SYSTEM OBJECTS](#5-file-system-objects)
- [6. DEVICE DRIVER OBJECTS](#6-device-driver-objects)
- [7. SECURITY OBJECTS](#7-security-objects)
- [8. COMMUNICATION OBJECTS](#8-communication-objects)
- [9. SCHEDULING OBJECTS](#9-scheduling-objects)
- [10. JIT COMPILER OBJECTS](#10-jit-compiler-objects)
- [11. DATABASE OBJECTS](#11-database-objects)
- [12. CONTAINER OBJECTS (NEBULA NATIVE)](#12-container-objects-nebula-native)
- [13. GRAPHICS AND UI OBJECTS](#13-graphics-and-ui-objects)
- [14. AUDIO OBJECTS](#14-audio-objects)
- [15. NETWORK OBJECTS](#15-network-objects)
- [16. POWER MANAGEMENT OBJECTS](#16-power-management-objects)
- [17. DEBUG AND PROFILING OBJECTS](#17-debug-and-profiling-objects)
- [18. SYSTEM SERVICE OBJECTS](#18-system-service-objects)
- [19. OBJECT COUNT SUMMARY](#19-object-count-summary)
- [20. COMPLETE OBJECT INHERITANCE TREE](#20-complete-object-inheritance-tree)
- [3. Object-Oriented Kernel Design](#3-object-oriented-kernel-design)
  - [3.1 Base Object Class](#31-base-object-class)
  - [3.2 Object Manager](#32-object-manager)
  - [3.3 Key Kernel Objects](#33-key-kernel-objects)
- [4. Box Type System](#4-box-type-system)
  - [4.1 Overview](#41-overview)
  - [4.2 Box Type Definition](#42-box-type-definition)
  - [4.3 Box Usage Examples](#43-box-usage-examples)
- [5. Unix Device Driver Integration](#5-unix-device-driver-integration)
  - [5.1 Legacy Driver Adapter](#51-legacy-driver-adapter)
  - [5.2 Driver Registration](#52-driver-registration)
  - [5.3 Supported Driver Types](#53-supported-driver-types)
- [6. Application Model](#6-application-model)
  - [6.1 Application Structure](#61-application-structure)
  - [6.2 Application Lifecycle](#62-application-lifecycle)
  - [6.3 Application Packaging](#63-application-packaging)
  - [6.4 Manifest Format](#64-manifest-format)
- [7. Just-In-Time Compilation System](#7-just-in-time-compilation-system)
  - [7.1 JIT Architecture](#71-jit-architecture)
  - [7.2 JIT Compiler Class](#72-jit-compiler-class)
  - [7.3 Expression and Statement AST](#73-expression-and-statement-ast)
  - [7.4 Optimization Passes](#74-optimization-passes)
  - [7.5 Multi-Language Support](#75-multi-language-support)
- [8. Memory Management](#8-memory-management)
  - [8.1 Memory Manager Architecture](#81-memory-manager-architecture)
  - [8.2 Allocation Strategies](#82-allocation-strategies)
  - [8.3 Object Pool](#83-object-pool)
  - [8.4 Box Memory Management](#84-box-memory-management)
- [9. Scheduling and Concurrency](#9-scheduling-and-concurrency)
  - [9.1 Task Class](#91-task-class)
  - [9.2 Scheduling Provisions](#92-scheduling-provisions)
  - [9.3 Scheduler](#93-scheduler)
  - [9.4 Concurrency Primitives](#94-concurrency-primitives)
- [10. File System and Security](#10-file-system-and-security)
  - [10.1 File System Hierarchy](#101-file-system-hierarchy)
  - [10.2 File System Objects](#102-file-system-objects)
  - [10.3 Security Profile](#103-security-profile)
  - [10.4 POSIX Compatibility](#104-posix-compatibility)
- [11. Desktop Environment](#11-desktop-environment)
  - [11.1 Desktop Manager](#111-desktop-manager)
  - [11.2 Window Manager](#112-window-manager)
  - [11.3 UI Element System](#113-ui-element-system)
  - [11.4 Predefined UI Elements](#114-predefined-ui-elements)
- [12. Database and Data Services](#12-database-and-data-services)
  - [12.1 Universal Database](#121-universal-database)
  - [12.2 MapDB (Memory-Mapped Database)](#122-mapdb-memory-mapped-database)
  - [12.3 WebTableSystem](#123-webtablesystem)
- [13. Build System and Manufacturing](#13-build-system-and-manufacturing)
  - [13.1 Image Builder](#131-image-builder)
  - [13.2 Build Configuration](#132-build-configuration)
  - [13.3 Manufacturing Workflow](#133-manufacturing-workflow)
- [14. API Reference](#14-api-reference)
  - [14.1 Core API](#141-core-api)
  - [14.2 JIT API](#142-jit-api)
  - [14.3 Database API](#143-database-api)
  - [14.4 Error Handling](#144-error-handling)
- [15. Appendices](#15-appendices)
  - [Appendix A: System Calls](#appendix-a-system-calls)
  - [Appendix B: File Types](#appendix-b-file-types)
  - [Appendix C: Environment Variables](#appendix-c-environment-variables)
  - [Appendix D: Glossary](#appendix-d-glossary)
- [Conclusion](#conclusion)

---

## 1. Introduction to NEBULA

### 1.1 What is NEBULA?

NEBULA (Native Extensible Binary Unified Layered Architecture) is a ground-up operating system

designed around object-oriented principles. Unlike traditional operating systems that bolt object-

oriented features onto procedural kernels, NEBULA makes objects the fundamental unit of system

abstraction.

### 1.2 Design Philosophy

| Principle | Description |
| --- | --- |
| Objects First | Everything is an object: processes, files, devices, windows, security |

```contexts
JIT Native
POSIX Compatible
No C Standard Library
Open Module Format
Fine-grained Loading
Manufacturer Ready
```

### 1.3 Key Features

```contexts
Object-oriented kernel with unified object management Box type system for dynamic typing across all subsystems Multi-language JIT supporting C, C++, Python, Go, JavaScript, and more Universal database layer with SQLite, MySQL, PostgreSQL, and Oracle support MapDB memory-mapped database for zero-copy operations WebTableSystem for text-based relational data management Unix device driver compatibility with object-oriented wrappers POSIX system call compatibility for existing applications Manufacturing pipeline for producing custom OS images Multiple platform support (x86_64, ARM64, RISC-V, Snapdragon)
```

## 2. System Architecture Overview

### 2.1 Layered Architecture

```text
┌───────────────────────────────────────────────────────────────────────────
──┐
```

```text
│                          APPLICATION LAYER
│
```

```text
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
```

```text
│
```

```text
│  │   Desktop   │ │ Web Server  │ │  Finance    │ │   Kiosk     │
```

```text
│
```

```text
│  │   Manager   │ │             │ │   Engine    │ │  Interface  │
```

```text
│
```

```text
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘
```

```text
│
```

```text
├───────────────────────────────────────────────────────────────────────────
──┤
```

```text
│                            SERVICE LAYER
│
```

```text
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
```

```text
│
```

```text
│  │ Universal   │ │ WebTable    │ │   MapDB     │ │ Multi-Lang  │
```

```text
│
```

```text
│  │     DB      │ │   System    │ │             │ │    JIT      │
```

```text
│
```

```text
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘
```

```text
│
```

```text
├───────────────────────────────────────────────────────────────────────────
──┤
```

```text
│                             CORE LAYER
│
```

```text
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
```

```text
│
```

```text
│  │    Box      │ │   Object    │ │  Scheduler  │ │ File System │
```

```text
│
```

```text
│  │   Types     │ │   Manager   │ │             │ │             │
```

```text
│
```

```text
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘
```

```text
│
```

```text
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
```

```text
│
```

```text
│  │   Memory    │ │  Security   │ │    JIT      │ │   POSIX     │
```

```text
│
```

```text
│  │  Management │ │   Profile   │ │   Engine    │ │  Compat     │
```

```text
│
```

```text
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘
```

```text
│
```

```text
├───────────────────────────────────────────────────────────────────────────
──┤
```

```text
│                           HARDWARE LAYER
│
```

```text
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
```

```text
│
```

```text
│  │   Intel     │ │    ARM      │ │   RISC-V    │ │ Snapdragon  │
```

```text
│
```

```text
│  │   x86_64    │ │   AArch64   │ │   64-bit    │ │    ARM      │
```

```text
│
```

```text
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘
```

```text
│
```

```text
└───────────────────────────────────────────────────────────────────────────
──┘
```

### 2.2 Component Interaction

| Component | Interacts With | Communication Method |
| --- | --- | --- |
| Box Type | All components | Direct value passing |
| Object Manager | All objects | ObjectID references |
| Scheduler | Tasks, Provisions | Context switching |
| File System | All objects | Object methods |
| Security Profile | FileSystemObject, Task | Permission checks |
| JIT Engine | Box, Expr, Stmt | Compiled code generation |
| Database | Box, JIT, FileSystem | Query execution |

## NEBULA Operating System

## Complete Kernel Object Directory

## 1. KERNEL OBJECT HIERARCHY OVERVIEW

```text
Object (Root)
```

```text
├── Kernel Core Objects
├── Task Management Objects
├── Memory Management Objects
├── File System Objects
├── Device Driver Objects
├── Security Objects
├── Communication Objects
├── Scheduling Objects
├── JIT Objects
├── Database Objects
├── Container Objects
├── Graphics Objects
├── Audio Objects
├── Network Objects
├── Power Management Objects
├── Debug Objects
└── System Service Objects
```

## 2. KERNEL CORE OBJECTS

| Object Name | Base Class | Description | Key Methods |
| --- | --- | --- | --- |
| Object | id() ,  ref() ,  unref() | None | Root of all kernel objects |
| ObjectManager | Object | registerObject() ,  res | Central object registry (singleton) |

```text
unregisterObject()
ObjectID
ObjectDirectory
Kernel
version()
System
hostname()
Configuration
Logger
error() ,  fatal()
Timer
interval()
Clock
InterruptController
disable() ,  eoi()
ExceptionHandler
BootInfo
framebuffer()
```

## 3. TASK MANAGEMENT OBJECTS

Object Name  Base  Description

Key Methods Class

```text
Task                      Object      Executable unit                run() ,  yield() ,  sleep() ,  wai
                                      (thread/process)               signal()
TaskGroup                 Object      Collection of related tasks    add() ,  remove() ,  killAll() ,  w
TaskID                    Object      Task identifier                value() ,  isValid() ,  toPid()
Thread                    Task        Lightweight execution unit     start() ,  join() ,  detach() ,  c
Process                   Task        Process with address space     fork() ,  exec() ,  exit() ,  wait
Fiber                     Task        User-space cooperative task    yield() ,  resume() ,  current()
TaskLocalStorage          Object      Per-task thread-local storage  get() ,  set() ,  clear()
TaskState                 Object      Saved task execution state     save() ,  restore() ,  switch()
RegisterFile              Object      CPU register context           save() ,  restore() ,  get() ,  se
SignalHandler             Object      POSIX signal management        deliver() ,  mask() ,  pending()
SignalSet                 Object      Collection of signals          add() ,  remove() ,  isEmpty() ,
```

## 4. MEMORY MANAGEMENT OBJECTS

```text
Object Name            Base Class           Description           Key Methods
MemoryManager           Object              Central memory         allocate() ,  free() ,
                                            management             map() ,  protect()
```

```text
(singleton)
MemoryRegion
,  reserve()
MemoryMap
removeMapping() ,
lookup()
PageAllocator
stats()
SlabAllocator
,  shrink()
PoolAllocator
reset()
BuddyAllocator
split() ,  merge()
SecureAllocator
freeSecure() ,  wipe()
DSPAllocator
freeAligned()
Heap
realloc() ,  calloc()
ObjectPool
warm() ,  drain()
BoxPool
recycleBox()
Buffer
slice()
BufferPool
capacity()
SharedMemory
attach() ,  detach()
MemoryProtection
setExec()
PageCache
flush() ,  invalidate()
VMADomain
,  protect()
AllocationStrateg
y
```

## 5. FILE SYSTEM OBJECTS

| Object Name | Base Class | Description | Key Methods |
| --- | --- | --- | --- |
| FileSystemObject | Object | read() ,  write() ,  stat( | Base file system object |

```text
security()
Directory
createDirectory() ,  remo
File
preallocate() ,  resize()
Device
select()
SymbolicLink
resolve()
HardLink
original()
Pipe
poll()
FIFO
close()
Socket
,  accept()
RootDirectory
resolveAbsolute()
MountPoint
filesystem()
Path
basename()
Stat
gid() ,  timestamps()
FileDescriptor
,  dup()
FileSystem
statfs()
Inode
permissions()
Storage
,  sync()
Volume
format() ,  repair()
Journal
rollback() ,  recover()
FileLock
,  held()
FileMonitor
onChange()
```

## 6. DEVICE DRIVER OBJECTS

| Object Name | Base Class | Description | Key Methods |
| --- | --- | --- | --- |
| Driver | Object | probe() , | Base device driver |

```text
initialize() , shutdown() , suspend() ,  re
DriverManager

findDriver() , loadDriver()
BlockDriver
sync() ,  ioctl
NetDriver

multicast() , promiscuous()
DisplayDriver

framebuffer()vsync() ,  blan
InputDriver
calibrate() ,
AudioDriver
mix() ,  volume
USBDriver

interrupt() , isochronous()
PCIDriver
irq() ,  dma()
LegacyDriverAdapter
emulate()
DeviceTree

parents() , children()
DeviceInfo
class() ,  subc
IOAddress

inw() ,  outw()inl() ,  outl()
MemoryMappedIO
offset() ,  siz
DMAChannel
wait() ,  abort
InterruptHandler

unregister() , enable() ,  dis
PowerManagement

state() , capabilities()
```

## 7. SECURITY OBJECTS

```text
Object Name                         Base Class            Description     Key Methods
SecurityProfile                      Object               Object           canRead() ,
                                                          security policy  canWrite() ,
```

```text
canExecute() ,
audit()
Capability
delegate() ,
revoke() ,
expiry()
CapabilitySet
has() ,  list()
AuditLog
,  export() ,
rotate()
AuditRecord
event() ,
subject() ,
object()
Permissions
mask() ,  inherit()
EncryptionKey
load() ,  store() ,
wipe()
Keyring
lookup() ,  clear()
```

```management
TaskCredentials
groups() ,
capabilities()
User
,  homeDir() ,
shell()
Group
groupname() ,
members()
Authentication
,  verify() ,
changePassword()
AccessControlList
removeEntry() ,
check()
SecurityPolicy
relax() ,
defaults()
MACLabel
category() ,
dominates()
Sealing
verify()
Attestation
,  measure()
TPM
,  seal() ,  unseal()Platform
Module
```

## 8. COMMUNICATION OBJECTS

```text
Object Name                 Base Class         Description                Key Methods
Port                         Object            Inter-object communication  send() ,  receive() ,  peek(
                                               port                        close()
Message                      Object            IPC message                 serialize() ,  deserialize(
```

```interface
reply() ,  forward()
Channel
close()
Signal
disconnect() ,  block()
Event
,  reset()
Queue
clear()
PriorityQueue
dequeueHighest()
Mailbox
flush()
SharedMemoryChannel
sync()
Socket
accept() ,  send() ,  recv()
UnixSocket
credentials()
NetlinkSocket
ack()
EventFD
Signalfd
Timerfd
expirations()
```

## 9. SCHEDULING OBJECTS

```text
Object Name                     Base Class          Description                Key Methods
Scheduler                        Object             Central scheduler           addTask() ,
                                                    (singleton)                 removeTask() ,
```

```interface
yield() ,  tick()
SchedulingDomain
setProvision() ,
affinity()
Provision
tick() ,  admit()
InteractiveProvision
fairShare() ,
priorityBoost()
RealTimeProvision
deadlineCheck()
,

admissionContro l()
BatchProvision
batchSize()
PowerSaveProvision

frequencyScale()
,  coalesce()
IsolationProvision
,  noSharing() ,
exclusive()
MixedProvision
migrate() ,
balance()
TimeConstraint
relative() ,
deadline() ,
slack()
AdmissionController
reject() ,

utilization() , schedulable()
LoadBalancer
migrateTask() ,
steal()
CPUAffinity
clear() ,
count()
Priority
inherit() ,
ceiling()
```

## 10. JIT COMPILER OBJECTS

| Object Name | Base Class | Description | Key Methods |
| --- | --- | --- | --- |
| JITCompiler | Service | compile() ,  execute() , | Just-in-time compiler |

```interface
optimize() ,  invalidate()
Expr
Call()
Stmt
Return()
Block
getVar() ,  setVar()
Optimizer
vectorize() ,  unroll()
OptimizationPass
invalidates()
ConstantFoldingPass
ss
ConstantPropagationPass
ss
DeadCodeEliminationPass
ss
StrengthReductionPass
ss
LoopUnrollingPass
ss
VectorizationPass
ss
InliningPass
ss
RegisterAllocationPass
ss
CodeEncoder
emitRet()
X86_64Encoder
emitRex()
ARM64Encoder
emitSIMD()
RISC_VEncoder
emitZba()
MultiLanguageJIT
registerAPI()
LanguageDetector
byKeywords()
Lexer
peek()
Parser
,  parseStatement()
TypeChecker
SymbolTable
scope()
```

## 11. DATABASE OBJECTS

Object Name  Base  Description

Key Methods Class

```text
UniversalDatabase            Servic     Main database      query() ,  execute() ,  insert() ,  begin()
                             e          interface
IDatabaseDriver              Object     Database driver    connect() ,  execute() ,  prepare() ,  back
```

```interface
SQLiteDriver
aseDri
ver
MySQLDriver
aseDri
ver
PostgreSQLDriver
aseDri
ver
OracleDriver
aseDri
ver
QueryBuilder
execute()
IRecordStream
```

```set
Row
```

```container
Column
```

```definition
PreparedStatement
```

```query
ConnectionPool
connection cache
BackupManager
```

```operations
Transaction
Table
Index
View
StoredProcedure
```

```procedure
Trigger
MapDB
e
MemoryMappedFile
```

```wrapper
RadixTrie
```

```structure
HashIndex
```

```index
JoinEngine
```

```operations
WebTable
relational table
Field
TextField
IntegerField
```

```field
DecimalField
decimal field
DateField
PictureField
```

```field
Visitor
```

```interface
HTMLVisitor
r
JSONVisitor
r
```

## 12. CONTAINER OBJECTS (NEBULA NATIVE)

Object  Base Class  Description

Key Methods Name

BoxVecto  Object  push() ,  pop() ,  at(  Box-optimized vector

```interface
r
BoxMap
BoxSet
ObjectCa
che
Property
Bag
EventBus
MessageQ
ueue
TaskQueu
e
Result
Future
Stream
Pipeline
Batch
RingBuff
er
BitMap
BloomFil
ter
HyperLog
Log
SkipList
RadixTre
e
DAG
Matrix
Tensor
SparseVe
ctor
SparseMa
trix
```

## 13. GRAPHICS AND UI OBJECTS

```text
Object Name                    Base Class           Description       Key Methods
DesktopManager                  Service             Main desktop       init() ,  run() ,
                                                    orchestrator       switchWorkspace()
WindowManager                   Object              Window             createWindow() ,
                                                    management         resize() ,  focus() ,
```

```interface
close()
X11WindowManager
icccm()
WaylandCompositor
xdgShell() ,
layerShell()
RenderBackend
drawText() ,  present()
OpenGLBackend
glTexture()
VulkanBackend
vkPipeline() ,
vkImage()
SoftwareBackend
fill()
SharedMemoryServer
writeDOM() ,
broadcast()
Workspace
removeWindow() ,
activate()
Panel
showClock() ,
showTray()
Launcher
launch()
NotificationDaemon
history()
Element
handleEvent() ,
layout()
Document
querySelector() ,
createElement()
Div
addClass() ,
removeClass()
Button
,  disable()
TextBox
onInput()
ListView
removeItem() ,  clear()
TreeView
collapse()
MenuBar
show()
Dialog
setModal()
TabControl
,  removeTab()
Canvas
drawRect() ,
drawImage()
Image
position()
Label
alignment() ,  wrap()
```

## 14. AUDIO OBJECTS

```text
Object Name              Base Class              Description               Key Methods
AudioManager              Service                Audio subsystem            play() ,  record() ,  mix()
                                                 manager                    route()
AudioDevice               Object                 Audio hardware device      open() ,  close() ,  start()
```

```interface
stop()
AudioStream
resume()
AudioBuffer
format()
Mixer
volume()
Effect
Codec
```

## 15. NETWORK OBJECTS

| Object Name | Base Class | Description | Key Methods |
| --- | --- | --- | --- |
| NetworkManager | Service | up() ,  down() ,  interfaces() , | Network subsystem manager |
| NetworkInterface | Object | name() ,  addresses() ,  mtu() , | Network interface |
| IPAddress | Object | toString() ,  family() ,  mask() | IP address (v4/v6) |
| Route | Object | destination() ,  gateway() ,  me | Network route |
| Socket | Object | bind() ,  connect() ,  send() , | Network socket endpoint |
| TCPSocket | Socket | listen() ,  accept() ,  sendStre | TCP stream socket |
| UDPSocket | Socket | sendTo() ,  recvFrom() ,  multic | UDP datagram socket |
| Packet | Object | header() ,  payload() ,  checksu | Network packet |
| Protocol | Object | handle() ,  send() ,  forward() | Network protocol handler |
| DNSResolver | Object | resolve() ,  reverse() ,  cache( | Domain name resolution |

## 16. POWER MANAGEMENT OBJECTS

```text
Object Name                      Base Class          Description                Key Methods
PowerManager                      Service            System power                suspend() ,  resume()
                                                     management                  reboot()
Battery                           Object             Battery status              level() ,  status() ,
```

```interface
timeRemaining() ,  hea
ThermalZone
passive()
FrequencyGovernor
WakeLock
```

## 17. DEBUG AND PROFILING OBJECTS

| Object Name | Base Class | Description | Key Methods |
| --- | --- | --- | --- |
| Debugger | Service | attach() ,  detach() , | System debugger |

```interface
breakpoint() ,  step()
Profiler
sample() ,  report()
Trace
annotate() ,  dump()
CrashDump
save()
KernelLog
clear() ,  level()
```

## 18. SYSTEM SERVICE OBJECTS

| Object Name | Base Class | Description | Key Methods |
| --- | --- | --- | --- |
| Service | Object | start() ,  stop() ,  status( | Base service interface |

```interface
reload()
ServiceManager
startAll() ,  stopAll()
SystemService
onCrash()
WebServer
handleRequest()
FinanceEngine
generateReport()
KioskInterface
showApp()
UpdateService
apply() ,  rollback()
TelemetryService
optOut()
```

## 19. OBJECT COUNT SUMMARY

| Category | Object Count |
| --- | --- |
| Kernel Core Objects | 15 |
| Task Management Objects | 13 |
| Memory Management Objects | 20 |
| File System Objects | 25 |
| Device Driver Objects | 18 |
| Security Objects | 20 |
| Communication Objects | 16 |
| Scheduling Objects | 14 |
| JIT Compiler Objects | 25 |
| Database Objects | 35 |
| Container Objects (NEBULA Native) | 24 |
| Graphics and UI Objects | 28 |
| Audio Objects | 7 |
| Network Objects | 11 |
| Power Management Objects | 6 |
| Debug and Profiling Objects | 6 |
| System Service Objects TOTAL | 8 291 |

## 20. COMPLETE OBJECT INHERITANCE TREE

```text
Object
```

```text
├── KernelCore
│   ├── ObjectManager
│   ├── ObjectDirectory
│   ├── Kernel
│   ├── System
│   ├── Configuration
│   ├── Logger
│   ├── Timer
│   ├── Clock
│   ├── InterruptController
│   ├── ExceptionHandler
│   └── BootInfo
├── TaskManagement
│   ├── Task
│   │   ├── Thread
│   │   ├── Process
│   │   └── Fiber
│   ├── TaskGroup
│   ├── TaskID
│   ├── TaskLocalStorage
│   ├── TaskState
│   ├── RegisterFile
│   ├── SignalHandler
│   └── SignalSet
├── MemoryManagement
│   ├── MemoryManager
│   ├── MemoryRegion
│   ├── MemoryMap
│   ├── PageAllocator
│   ├── SlabAllocator
│   ├── PoolAllocator
│   ├── BuddyAllocator
│   ├── SecureAllocator
│   ├── DSPAllocator
│   ├── Heap
│   ├── ObjectPool
│   │   └── BoxPool
│   ├── Buffer
│   ├── BufferPool
│   ├── SharedMemory
│   ├── MemoryProtection
│   ├── PageCache
│   ├── VMADomain
│   └── AllocationStrategy
├── FileSystem
│   ├── FileSystemObject
│   │   ├── Directory
│   │   ├── File
│   │   ├── Device
│   │   ├── SymbolicLink
│   │   ├── HardLink
│   │   ├── Pipe
│   │   ├── FIFO
│   │   └── Socket
│   ├── RootDirectory
│   ├── MountPoint
│   ├── Path
│   ├── Stat
│   ├── FileDescriptor
│   ├── FileSystem
│   ├── Inode
│   ├── Storage
│   ├── Volume
│   ├── Journal
│   ├── FileLock
│   └── FileMonitor
├── DeviceDrivers
│   ├── Driver
│   │   ├── BlockDriver
│   │   ├── NetDriver
│   │   ├── DisplayDriver
│   │   ├── InputDriver
│   │   ├── AudioDriver
│   │   ├── USBDriver
│   │   ├── PCIDriver
│   │   └── LegacyDriverAdapter
│   ├── DriverManager
│   ├── DeviceTree
│   ├── DeviceInfo
│   ├── IOAddress
│   ├── MemoryMappedIO
│   ├── DMAChannel
│   ├── InterruptHandler
│   └── PowerManagement
├── Security
│   ├── SecurityProfile
│   ├── Capability
│   ├── CapabilitySet
│   ├── AuditLog
│   ├── AuditRecord
│   ├── Permissions
│   ├── EncryptionKey
│   ├── Keyring
│   ├── TaskCredentials
│   ├── User
│   ├── Group
│   ├── Authentication
│   ├── AccessControlList
│   ├── SecurityPolicy
│   ├── MACLabel
│   ├── Sealing
│   ├── Attestation
│   └── TPM
├── Communication
│   ├── Port
│   ├── Message
│   ├── Channel
│   │   └── SharedMemoryChannel
│   ├── Signal
│   ├── Event
│   ├── Queue
│   │   └── PriorityQueue
│   ├── Mailbox
│   ├── Socket
│   │   ├── UnixSocket
│   │   └── NetlinkSocket
│   ├── EventFD
│   ├── Signalfd
│   └── Timerfd
├── Scheduling
│   ├── Scheduler
│   ├── SchedulingDomain
│   ├── Provision
│   │   ├── InteractiveProvision
│   │   ├── RealTimeProvision
│   │   ├── BatchProvision
│   │   ├── PowerSaveProvision
│   │   ├── IsolationProvision
│   │   └── MixedProvision
│   ├── TimeConstraint
│   ├── AdmissionController
│   ├── LoadBalancer
│   ├── CPUAffinity
│   └── Priority
├── JIT
│   ├── JITCompiler
│   │   └── MultiLanguageJIT
│   ├── Expr
│   ├── Stmt
│   ├── Block
│   ├── Optimizer
│   ├── OptimizationPass
│   │   ├── ConstantFoldingPass
│   │   ├── ConstantPropagationPass
│   │   ├── DeadCodeEliminationPass
│   │   ├── StrengthReductionPass
│   │   ├── LoopUnrollingPass
│   │   ├── VectorizationPass
│   │   ├── InliningPass
│   │   └── RegisterAllocationPass
│   ├── CodeEncoder
│   │   ├── X86_64Encoder
│   │   ├── ARM64Encoder
│   │   └── RISC_VEncoder
│   ├── LanguageDetector
│   ├── Lexer
│   ├── Parser
│   ├── TypeChecker
│   └── SymbolTable
├── Database
│   ├── UniversalDatabase
│   ├── IDatabaseDriver
│   │   ├── SQLiteDriver
│   │   ├── MySQLDriver
│   │   ├── PostgreSQLDriver
│   │   └── OracleDriver
│   ├── QueryBuilder
│   ├── IRecordStream
│   ├── Row
│   ├── Column
│   ├── PreparedStatement
│   ├── ConnectionPool
│   ├── BackupManager
│   ├── Transaction
│   ├── Table
│   ├── Index
│   ├── View
│   ├── StoredProcedure
│   ├── Trigger
│   ├── MapDB
│   ├── MemoryMappedFile
│   ├── RadixTrie
│   ├── HashIndex
│   ├── JoinEngine
│   ├── WebTable
│   ├── Field
│   │   ├── TextField
│   │   ├── IntegerField
│   │   ├── DecimalField
│   │   ├── DateField
│   │   └── PictureField
│   └── Visitor
│       ├── HTMLVisitor
│       └── JSONVisitor
├── Containers
│   ├── BoxVector
│   ├── BoxMap
│   ├── BoxSet
│   ├── ObjectCache
│   ├── PropertyBag
│   ├── EventBus
│   ├── MessageQueue
│   ├── TaskQueue
│   ├── Result
│   ├── Future
│   ├── Stream
│   ├── Pipeline
│   ├── Batch
│   ├── RingBuffer
│   ├── BitMap
│   ├── BloomFilter
│   ├── HyperLogLog
│   ├── SkipList
│   ├── RadixTree
│   ├── DAG
│   ├── Matrix
│   ├── Tensor
│   ├── SparseVector
│   └── SparseMatrix
├── GraphicsUI
│   ├── DesktopManager
│   ├── WindowManager
│   │   ├── X11WindowManager
│   │   └── WaylandCompositor
│   ├── RenderBackend
│   │   ├── OpenGLBackend
│   │   ├── VulkanBackend
│   │   └── SoftwareBackend
│   ├── SharedMemoryServer
│   ├── Workspace
│   ├── Panel
│   ├── Launcher
│   ├── NotificationDaemon
│   └── Element
│       ├── Document
│       ├── Div
│       ├── Button
│       ├── TextBox
│       ├── ListView
│       ├── TreeView
│       ├── MenuBar
│       ├── Dialog
│       ├── TabControl
│       ├── Canvas
│       ├── Image
│       └── Label
├── Audio
│   ├── AudioManager
│   ├── AudioDevice
│   ├── AudioStream
│   ├── AudioBuffer
│   ├── Mixer
│   ├── Effect
│   └── Codec
├── Network
│   ├── NetworkManager
│   ├── NetworkInterface
│   ├── IPAddress
│   ├── Route
│   ├── Socket
│   │   ├── TCPSocket
│   │   └── UDPSocket
│   ├── Packet
│   ├── Protocol
│   └── DNSResolver
├── PowerManagement
│   ├── PowerManager
│   ├── Battery
│   ├── ThermalZone
│   ├── FrequencyGovernor
│   └── WakeLock
├── Debug
│   ├── Debugger
│   ├── Profiler
│   ├── Trace
│   ├── CrashDump
│   └── KernelLog
└── Services
    ├── Service
    ├── ServiceManager
    ├── SystemService
    ├── WebServer
    ├── FinanceEngine
    ├── KioskInterface
    ├── UpdateService
    └── TelemetryService
```

Documentation Version 5.0

Copyright (c) 2026 Anthony Matarazzo

Total Kernel Objects: 291

## 3. Object-Oriented Kernel Design

### 3.1 Base Object Class

All kernel entities inherit from the base  Object  class:

```cpp
class Object {
private:
    ObjectID _id;
    std::string _name;
    std::atomic<uint32_t> _refCount;
    SecurityProfile* _security;

public:
    Object();
    virtual ~Object();

    ObjectID id() const;
    const std::string& name() const;
    void setName(const std::string& name);

    void ref();
    void unref();
    uint32_t refCount() const;

    SecurityProfile* security() const;
    void setSecurity(SecurityProfile* profile);

    virtual std::string toString() const;
    virtual bool equals(const Object* other) const;
};
```

### 3.2 Object Manager

The central registry for all kernel objects:

```cpp
class ObjectManager : public Object {
private:
    std::unordered_map<ObjectID, Object*> _objects;
    std::unordered_map<std::string, ObjectID> _nameIndex;
    std::mutex _mutex;
    static ObjectManager* _instance;

public:
    static ObjectManager& instance();
    ObjectID registerObject(Object* obj);
    void unregisterObject(ObjectID id);
    Object* resolve(ObjectID id);

    template<typename T>
    T* resolve(ObjectID id) {
        return dynamic_cast<T*>(resolve(id));
    }

    Object* findByName(const std::string& name);
    std::vector<ObjectID> listObjects() const;
};
```

### 3.3 Key Kernel Objects

| Object Type | Description | Example |
| --- | --- | --- |
| Task | Executable unit | User application |

```cpp
(thread/process)
Scheduler
FileSystemObject
Device
Driver
Port
Timer
Signal
```

## 4. Box Type System

### 4.1 Overview

The Box type system is NEBULA's universal data container. Every value in the system can be

stored, passed, and manipulated as a Box.

### 4.2 Box Type Definition

```cpp
class Box {
public:
    enum class Type {
        VOID, BOOL, I8, U8, I16, U16, I32, U32, I64, U64,
        F32, F64, STRING, PTR, VECTOR, LIST, MAP, SET,
        FUNCTION, OBJECT, CLASS, METHOD,
        DATABASE, TABLE, JIT_COMPILER, BUFFER
    };

private:
    Type _type;
    union {
        bool b;
        int8_t i8; uint8_t u8;
        int16_t i16; uint16_t u16;
        int32_t i32; uint32_t u32;
        int64_t i64; uint64_t u64;
        float f32; double f64;
        void* ptr;
    };
    std::shared_ptr<void> _shared;

public:
    // Constructors
    Box();
    Box(bool value);
    Box(int64_t value);
    Box(double value);
    Box(const std::string& value);
    Box(const char* value);

    // Type conversion
    bool asBool() const;
    int64_t asInt64() const;
    double asDouble() const;
    std::string asString() const;

    // Operators
    Box operator+(const Box& other) const;
    Box operator-(const Box& other) const;
    Box operator*(const Box& other) const;
    Box operator/(const Box& other) const;
    Box operator&(const Box& other) const;  // Bitwise AND
    Box operator|(const Box& other) const;  // Bitwise OR
    Box operator^(const Box& other) const;  // Bitwise XOR
    Box operator~() const;                   // Bitwise NOT

    // Container operations
    static Box makeVector();
    static Box makeMap();
    void push_back(const Box& value);
    Box& operator[](size_t index);
};
```

### 4.3 Box Usage Examples

```cpp
// Basic types
Box b1 = Box(true);           // Boolean
Box b2 = Box(42);             // Integer
Box b3 = Box(3.14159);        // Double
Box b4 = Box("Hello");        // String

// Arithmetic
Box sum = b2 + Box(8);        // 50
Box product = b2 * Box(2);    // 84

// Bitwise operations
Box flags = Box(0b1010);
Box mask = Box(0b1100);
Box result = flags & mask;    // 0b1000

// Containers
Box vec = Box::makeVector();
vec.push_back(Box(1));
vec.push_back(Box(2));
vec.push_back(Box(3));

// Dynamic typing
Box value = Box(42);
if (value.type() == Box::Type::I64) {
    int64_t n = value.asInt64();
}
```

## 5. Unix Device Driver Integration

### 5.1 Legacy Driver Adapter

NEBULA supports existing Unix device drivers through an object-oriented adapter pattern:

```cpp
// Legacy C driver structure (cannot be changed)
struct legacy_block_driver {
    void (*init)(uint32_t device_id);
    int (*read)(uint32_t sector, uint8_t* buffer, size_t count);
    int (*write)(uint32_t sector, const uint8_t* buffer, size_t count);
    void (*shutdown)(void);
};

// NEBULA object-oriented wrapper
class LegacyBlockDriverAdapter : public BlockDriver {
private:
    legacy_block_driver* _legacy;
    uint32_t _device_id;

public:
    LegacyBlockDriverAdapter(legacy_block_driver* driver, uint32_t id)
        : _legacy(driver), _device_id(id) {
        _legacy->init(_device_id);
    }

    ~LegacyBlockDriverAdapter() {
        _legacy->shutdown();
    }

    Result<Buffer> read(uint64_t sector, size_t count) override {
        Buffer buf(count * 512);
        int ret = _legacy->read(sector, buf.data(), count);
        if (ret == 0) {
            return buf;
        }
        return Error::fromCode(ret);
    }

    Result<size_t> write(uint64_t sector, const Buffer& data) override {
        return _legacy->write(sector, data.data(), data.size() / 512);
    }
};
```

### 5.2 Driver Registration

```cpp
class DriverManager : public Object {
private:
    std::unordered_map<std::string, std::vector<Driver*>> _drivers;
    std::vector<Driver*> _legacy_adapters;

public:
    void registerDriver(Driver* driver);
    void registerLegacyDriver(const std::string& name, void* legacy_struct);

    template<typename T>
    T* getDriver(const std::string& name) {
        auto it = _drivers.find(name);
        if (it != _drivers.end() && !it->second.empty()) {
            return dynamic_cast<T*>(it->second[0]);
        }
        return nullptr;
    }
    void probeAll();
};
```

### 5.3 Supported Driver Types

| Driver Type | Interface | Legacy Support |
| --- | --- | --- |
| read() ,  write() , | BlockDriver | Linux block driver |

```cpp
sync()
send() ,  receive() ,
multicast()
setMode() ,
framebuffer() ,
vsync()
readEvent() ,  poll() ,
calibrate()
play() ,  record() ,
mix()
control() ,  bulk() ,
interrupt()
```

## 6. Application Model

### 6.1 Application Structure

NEBULA applications are object-oriented and can be written in any supported language:

```cpp
class Application : public Object {
protected:
    std::string _name;
    std::string _version;
    std::shared_ptr<Block> _mainBlock;

public:
    Application(const std::string& name, const std::string& version);
    virtual ~Application();

    virtual bool initialize() = 0;
    virtual bool run() = 0;
    virtual void shutdown() = 0;

    const std::string& name() const { return _name; }
    const std::string& version() const { return _version; }
};
// Example application
class MyApp : public Application {
public:
    MyApp() : Application("MyApp", "1.0.0") {}

    bool initialize() override {
        // Setup code
        return true;
    }

    bool run() override {
        // Main application loop
        return true;
    }

    void shutdown() override {
        // Cleanup
    }
};
```

### 6.2 Application Lifecycle

```text
┌─────────────┐
│   LOADED    │  Application image loaded into memory
└──────┬──────┘
```

```text
       ▼
```

```text
┌─────────────┐
│ INITIALIZE  │  Constructor, object creation, resource allocation
└──────┬──────┘
```

```text
       ▼
```

```text
┌─────────────┐
│   RUNNING   │  Main loop, event processing
└──────┬──────┘
```

```text
       ▼
```

```text
┌─────────────┐
│  SUSPENDED  │  Paused (background, waiting)
└──────┬──────┘
```

```text
       ▼
```

```text
┌─────────────┐
│  SHUTDOWN   │  Cleanup, resource release
└──────┬──────┘
```

```text
       ▼
```

```text
┌─────────────┐
│  TERMINATED │  Process ended
└─────────────┘
```

### 6.3 Application Packaging

NEBULA applications are packaged as  .nebula-app  files:

```text
myapp.nebula-app/
```

```text
├── manifest.json          # Application metadata
├── main.nebula            # Compiled bytecode or native code
├── resources/
│   ├── icons/
│   ├── locales/
│   └── data/
├── lib/                   # Shared libraries
└── config/                # Default configuration
```

### 6.4 Manifest Format

```json
{
  "app_id": "com.example.myapp",
  "name": "My Application",
  "version": "1.0.0",
  "author": "Developer Name",
  "entry_point": "main.nebula",
  "permissions": ["filesystem.read", "network.connect", "graphics.render"],
  "dependencies": [
    {"name": "nebula-runtime", "version": ">=5.0"}
  ],
  "desktop": {
    "icon": "resources/icons/app.png",
    "categories": ["Utility", "Development"],
    "terminal": false
  }
}
```

## 7. Just-In-Time Compilation System

### 7.1 JIT Architecture

```text
┌───────────────────────────────────────────────────────────────────────────
──┐
```

```text
│                           JIT COMPILER
│
```

```text
├───────────────────────────────────────────────────────────────────────────
──┤
```

```text
│
│
```

```text
│  Source Code ──→ Lexer ──→ Parser ──→ AST ──→ Optimizer ──→ CodeGen ──→
```

```text
Exec│
│     (any          (tokens)     (Expr/      (multiple    (machine
(native  │
│    language)                   Stmt)        passes)       code)     run)
│
```

```text
│                                                                              │
└───────────────────────────────────────────────────────────────────────────
──┘
```

### 7.2 JIT Compiler Class

```cpp
class JITCompiler : public Service {
private:
    CPUType _cpu;
    OptimizationLevel _optLevel;
    std::shared_ptr<Block> _currentBlock;
    std::unordered_map<std::string, Box> _globals;
    std::unique_ptr<CodeEncoder> _encoder;

public:
    JITCompiler(CPUType cpu = CPUType::X86_64,
                OptimizationLevel opt = OptimizationLevel::O2);

    // Compilation
    void addStatement(std::shared_ptr<Stmt> stmt);
    void addExpression(std::shared_ptr<Expr> expr);
    void compile();
    Box execute();

    // Optimization
    std::shared_ptr<Expr> optimize(std::shared_ptr<Expr> expr);
    void setOptimizationLevel(OptimizationLevel level);

    // Variable management
    void setGlobal(const std::string& name, const Box& value);
    Box getGlobal(const std::string& name);

    // Function registration
    void registerFunction(const std::string& name, void* fn);
    void registerCallback(const std::string& name,
                          std::function<Box(std::vector<Box>&)> callback);

    // Multi-language support
    void parseSource(const std::string& source, Language lang);
    void setLanguage(Language lang);
};
```

### 7.3 Expression and Statement AST

```cpp
// Expression types
class Expr {
public:
    enum class Kind { CONST, VAR, BINOP, UNARYOP, CALL, MEMBER, NEW };

    static std::shared_ptr<Expr> Const(const Box& value);
    static std::shared_ptr<Expr> Var(const std::string& name);
    static std::shared_ptr<Expr> BinOp(BinOp op, ExprPtr lhs, ExprPtr rhs);
    static std::shared_ptr<Expr> Call(const std::string& name,
                                       const std::vector<ExprPtr>& args);
};

// Statement types
class Stmt {
public:
    enum class Kind { EXPR, ASSIGN, IF, WHILE, FOR, RETURN, BLOCK };

    static std::shared_ptr<Stmt> Expr(ExprPtr expr);
    static std::shared_ptr<Stmt> Assign(const std::string& var, ExprPtr
value);
    static std::shared_ptr<Stmt> If(ExprPtr cond, BlockPtr thenBlock,
                                     BlockPtr elseBlock = nullptr);
    static std::shared_ptr<Stmt> While(ExprPtr cond, BlockPtr body);
    static std::shared_ptr<Stmt> Return(ExprPtr value = nullptr);
};
```

### 7.4 Optimization Passes

| Pass Name | Description | Example Transformation |
| --- | --- | --- |
| 2 + 3  →  5 | ConstantFolding | Evaluate constants at |

compile time

x = 5; y = x + 3  →  y = 8  ConstantPropagation  Replace variables

with constants

```text
DeadCodeElimination         Remove unreachable     if (false) { ... }  →
                            code                  removed
StrengthReduction           Replace expensive      x * 2  →  x << 1
```

```ops
for(i=0;i<4;i++)  → inline 4x
```

```vectorization
Vectorization
f(x)  → body of f
```

```bodies
RegisterAllocation
```

### 7.5 Multi-Language Support

NEBULA's JIT supports multiple programming languages:

| Language | Lexer | Parser | Runtime | Use Case |
| --- | --- | --- | --- | --- |
| C | Full | Full | Native | System code |
| C++ | Full | Full | Native | OO applications |
| Python | Full | Full | Interpreter | Scripting |
| JavaScript | Full | Full | Interpreter | Web apps |
| Go | Full | Full | Native | Concurrent code |
| BASIC | Full | Full | Interpreter | Legacy apps |
| SQL | Partial | Full | Database | Queries |

```cpp
// Multi-language JIT usage
MultiLanguageJIT jit;
jit.parse(source_code, Language::PYTHON);
jit.execute();

// Or auto-detect
jit.parse(source_code);  // Auto-detects language
```

## 8. Memory Management

### 8.1 Memory Manager Architecture

```cpp
class MemoryManager : public Object {
private:
    std::unique_ptr<AllocationStrategy> _strategy;
    std::unordered_map<void*, size_t> _allocations;
    size_t _totalAllocated;
    size_t _peakAllocated;

public:
    static MemoryManager& instance();

    void* allocate(size_t size);
    void free(void* ptr);
    void* reallocate(void* ptr, size_t newSize);

    void setStrategy(std::unique_ptr<AllocationStrategy> strategy);

    // Object pool
    template<typename T>
    T* allocateObject() {
        return new (_allocate(sizeof(T))) T();
    }

    template<typename T>
    void deallocateObject(T* obj) {
        obj->~T();
        _free(obj);
    }

    // Statistics
    size_t totalAllocated() const;
    size_t peakAllocated() const;
    size_t fragmentation() const;
};
```

### 8.2 Allocation Strategies

| Strategy | Description | Use Case |
| --- | --- | --- |
| PoolAllocator | Fixed-size blocks | Object pools |
| BuddyAllocator | Power-of-two blocks | Kernel memory |
| SlabAllocator | Cached objects | Frequent allocations |
| SecureAllocator | Zero on free | Cryptographic data |
| DSPAllocator | Cache-aligned | Audio/video processing |

### 8.3 Object Pool

```cpp
template<typename T>
class ObjectPool {
private:
    std::vector<T*> _free;
    std::vector<T*> _used;
    size_t _poolSize;
    std::function<T*()> _factory;

public:
    ObjectPool(size_t initialSize, std::function<T*()> factory);
    ~ObjectPool();

    T* acquire();
    void release(T* obj);

    size_t available() const { return _free.size(); }
    size_t used() const { return _used.size(); }
};
```

### 8.4 Box Memory Management

Box objects use reference counting for automatic memory management:

```cpp
class Box {
private:
    struct ControlBlock {
        std::atomic<uint32_t> refCount;
        Type type;
        size_t size;
    };
    ControlBlock* _control;
    void* _data;

public:
    Box(const Box& other) {
        _control = other._control;
        _data = other._data;
        _control->refCount++;
    }

    ~Box() {
        if (_control && --_control->refCount == 0) {
            MemoryManager::instance().free(_control);
        }
    }

    Box& operator=(const Box& other) {
        if (this != &other) {
            this->~Box();
            new (this) Box(other);
        }
        return *this;
    }
};
```

## 9. Scheduling and Concurrency

### 9.1 Task Class

```cpp
class Task : public Object {
private:
    TaskID _id;
    TaskState _state;
    std::unique_ptr<RegisterFile> _registers;
    std::shared_ptr<Block> _code;
    SecurityProfile* _security;
    uint64_t _quantum;
    uint64_t _runtime;

public:
    Task(std::shared_ptr<Block> code);
    ~Task();

    void run();
    void yield();
    void sleep(uint64_t microseconds);
    void wait(Object* object);
    void signal(Object* object);

    TaskState state() const { return _state; }
    TaskID id() const { return _id; }

    // Clone for fork()
    std::unique_ptr<Task> clone();
};
```

### 9.2 Scheduling Provisions

```cpp
class Provision : public Object {
public:
    virtual Task* selectNext(std::vector<Task*>& ready) = 0;
    virtual void tick(Task* current) = 0;
    virtual bool admit(Task* task) = 0;
};
// Interactive (desktop) provision
class InteractiveProvision : public Provision {
private:
    std::map<TaskID, uint64_t> _vruntime;

public:
    Task* selectNext(std::vector<Task*>& ready) override {
        // Select task with smallest virtual runtime
        return *std::min_element(ready.begin(), ready.end(),
            [this](Task* a, Task* b) {
                return _vruntime[a->id()] < _vruntime[b->id()];
            });
    }
};

// Real-time provision (Earliest Deadline First)
class RealTimeProvision : public Provision {
private:
    std::map<TaskID, uint64_t> _deadlines;

public:
    bool admit(Task* task) override {
        // Schedulability test
        return calculateUtilization() <= 1.0;
    }

    Task* selectNext(std::vector<Task*>& ready) override {
        // Select task with earliest deadline
        return *std::min_element(ready.begin(), ready.end(),
            [this](Task* a, Task* b) {
                return _deadlines[a->id()] < _deadlines[b->id()];
            });
    }
};
```

### 9.3 Scheduler

```cpp
class Scheduler : public Object {
private:
    std::vector<std::unique_ptr<SchedulingDomain>> _domains;
    std::unordered_map<TaskID, Task*> _allTasks;
    std::queue<Task*> _readyQueue;
    static Scheduler* _instance;

public:
    static Scheduler& instance();
    void addTask(std::unique_ptr<Task> task);
    void removeTask(TaskID id);

    void yield();
    void yieldUntil(const TimePoint& deadline);

    void setProvision(int core, Provision* provision);
    void setAffinity(Task* task, int core);

    // Called on timer interrupt
    void tick();
};
```

### 9.4 Concurrency Primitives

| Primitive | Class | Description |
| --- | --- | --- |
| Mutex | Mutex | Mutual exclusion lock |
| Spinlock | Spinlock | Busy-wait lock for short |

```sections
Semaphore
ConditionVar
Barrier
RWLock
```

## 10. File System and Security

### 10.1 File System Hierarchy

```text
/                           # Root directory
```

```text
├── system/                 # System objects
│   ├── kernel/            # Kernel objects
│   ├── drivers/           # Device drivers
│   └── services/          # System services
├── dev/                    # Device files
│   ├── null
│   ├── zero
│   ├── random
│   └── tty0
├── proc/                   # Process information
├── home/                   # User home directories
├── etc/                    # Configuration
├── var/                    # Variable data
│   ├── log/
│   └── spool/
├── usr/                    # User programs
│   ├── bin/
│   ├── lib/
│   └── share/
└── tmp/                    # Temporary files
```

### 10.2 File System Objects

```cpp
class FileSystemObject : public Object {
protected:
    Directory* _parent;
    std::string _name;
    SecurityProfile* _security;
    Timestamp _created;
    Timestamp _modified;

public:
    virtual Result<Buffer> read(uint64_t offset, size_t size) = 0;
    virtual Result<size_t> write(uint64_t offset, const Buffer& data) = 0;
    virtual Result<Stat> stat() = 0;

    void setSecurity(SecurityProfile* profile);
    SecurityProfile* security() const;
};

class Directory : public FileSystemObject {
private:
    std::unordered_map<std::string, ObjectID> _children;

public:
    Result<FileSystemObject*> lookup(const std::string& name);
    Result<File*> createFile(const std::string& name);
    Result<Directory*> createDirectory(const std::string& name);
    void remove(const std::string& name);
    std::vector<std::string> list() const;
};

class File : public FileSystemObject {
private:
    std::shared_ptr<Storage> _storage;
    uint64_t _size;

public:
    Result<Buffer> read(uint64_t offset, size_t size) override;
    Result<size_t> write(uint64_t offset, const Buffer& data) override;
    void truncate(uint64_t newSize);
    void sync();
};
```

### 10.3 Security Profile

```cpp
class SecurityProfile : public Object {
private:
    Permissions _permissions;
    std::vector<Capability> _capabilities;
    AuditLevel _audit;
    std::array<uint8_t, 32> _encryptionKey;

public:
    // Permission checks
    bool canRead(const Task* task) const;
    bool canWrite(const Task* task) const;
    bool canExecute(const Task* task) const;
    bool canAdmin(const Task* task) const;

    // Audit
    void auditRead(const FileSystemObject* obj, uint64_t offset, size_t
size);
    void auditWrite(const FileSystemObject* obj, uint64_t offset, size_t
size);
    void auditAuth(const Task* task, const std::string& operation);

    // Encryption
    Buffer encrypt(const Buffer& data) const;
    Buffer decrypt(const Buffer& data) const;

    // Factory methods
    static SecurityProfile* ownerOnly();
    static SecurityProfile* readOnly();
    static SecurityProfile* fullAccess();
    static SecurityProfile* secure();
};
```

### 10.4 POSIX Compatibility

```cpp
class POSIXSubsystem : public Object {
private:
    std::unordered_map<int, ObjectID> _fdTable;
    std::unordered_map<int, Capability> _fdCaps;
    int _nextFd;
public:
    // POSIX system calls
    int open(const std::string& path, int flags, mode_t mode);
    ssize_t read(int fd, void* buf, size_t count);
    ssize_t write(int fd, const void* buf, size_t count);
    int close(int fd);
    off_t lseek(int fd, off_t offset, int whence);

    // Process management
    pid_t fork();
    int execve(const char* path, char* const argv[], char* const envp[]);
    pid_t waitpid(pid_t pid, int* status, int options);

    // File system
    int stat(const char* path, struct stat* buf);
    int chmod(const char* path, mode_t mode);
    int chown(const char* path, uid_t owner, gid_t group);
};
```

## 11. Desktop Environment

### 11.1 Desktop Manager

```cpp
class DesktopManager : public Service {
private:
    std::unique_ptr<WindowManager> _wm;
    std::unique_ptr<RenderBackend> _renderer;
    std::unique_ptr<SharedMemoryServer> _shm;
    std::vector<std::unique_ptr<Workspace>> _workspaces;
    int _currentWorkspace;

public:
    bool initialize() override;
    bool run() override;
    void shutdown() override;

    void createWindow(ObjectID clientId, int x, int y, int w, int h);
    void destroyWindow(ObjectID clientId);
    void focusWindow(ObjectID clientId);

    void switchWorkspace(int workspace);
    void showNotification(const std::string& title, const std::string&
message);
    void showLauncher();
};
```

### 11.2 Window Manager

```cpp
class WindowManager : public Object {
public:
    virtual void createWindow(uint32_t clientId, int x, int y, int w, int h)
= 0;
    virtual void destroyWindow(uint32_t clientId) = 0;
    virtual void resizeWindow(uint32_t clientId, int w, int h) = 0;
    virtual void moveWindow(uint32_t clientId, int x, int y) = 0;
    virtual void focusWindow(uint32_t clientId) = 0;

    // X11 implementation
    static std::unique_ptr<WindowManager> createX11();

    // Wayland implementation
    static std::unique_ptr<WindowManager> createWayland();
};
```

### 11.3 UI Element System

```cpp
class Element : public Object {
protected:
    Element* _parent;
    std::vector<std::unique_ptr<Element>> _children;
    std::unordered_map<std::string, Box> _properties;
    std::unordered_map<std::string, EventHandler> _handlers;

public:
    virtual void render(RenderBackend* backend) = 0;
    virtual bool handleEvent(const Event& event) = 0;
    virtual void layout(const Rect& bounds) = 0;

    void addChild(std::unique_ptr<Element> child);
    void removeChild(Element* child);

    template<typename T>
    void setProperty(const std::string& name, const T& value) {
        _properties[name] = Box(value);
    }

    template<typename T>
    T getProperty(const std::string& name) const {
        auto it = _properties.find(name);
        if (it != _properties.end()) {
            return it->second.as<T>();
        }
        return T();
    }

    void on(const std::string& event, EventHandler handler);
    void emit(const std::string& event, const Box& data);
};
```

### 11.4 Predefined UI Elements

| Element | Description | Properties |
| --- | --- | --- |
| Button | Clickable button | text, icon, enabled |
| TextBox | Text input field | text, placeholder, |

maxLength

| Label | Static text | text, alignment, wrap |
| --- | --- | --- |
| ListView | Scrollable list | items, selection, |

```multiselect
TreeView
```

```selection
MenuBar
ToolBar
Dialog
Canvas
Image
```

## 12. Database and Data Services

### 12.1 Universal Database

```cpp
class UniversalDatabase : public Service {
private:
    std::unique_ptr<IDatabaseDriver> _driver;
    std::unique_ptr<ConnectionPool> _pool;

public:
    UniversalDatabase(const std::string& driverType, const std::string&
connString);

    // Query execution
    std::unique_ptr<IRecordStream> query(const std::string& sql);
    std::unique_ptr<IRecordStream> query(const QueryBuilder& qb);

    // Data manipulation
    void insert(const std::string& table, const Row& row);
    void update(const std::string& table, const Row& row, const std::string&
where);
    void remove(const std::string& table, const std::string& where);

    // Schema
    void createTable(const std::string& name, const std::vector<Column>&
columns);
    void dropTable(const std::string& name);

    // Transactions
    void begin();
    void commit();
    void rollback();

    // File import/export
    void loadCSV(const std::string& filename, const std::string& table);
    void exportCSV(const std::string& table, const std::string& filename);
};
```

### 12.2 MapDB (Memory-Mapped Database)

```cpp
template<typename Record>
class MapDBTable : public Object {
private:
    std::unique_ptr<mapdb::Storage<Record>> _storage;
    std::unique_ptr<mapdb::HashIndex<typename Record::KeyType>> _index;
    std::unique_ptr<mapdb::RadixTrie> _trie;

public:
    MapDBTable(const std::string& filename, size_t size);

    void insert(const Record& record);
    Record* find(const typename Record::KeyType& key);
    std::vector<Record*> prefix(const std::string& prefix);

    template<typename Predicate>
    std::vector<Record*> where(Predicate pred) {
        // Parallel filtered scan
    }

    size_t size() const;
    void sync();
};
```

### 12.3 WebTableSystem

```cpp
class WebTable : public Object {
private:
    std::string _schemaFile;
    std::vector<std::string> _dataFiles;
    std::vector<Record> _records;
    std::map<std::string, std::map<std::string, std::string>> _indexes;

public:
    void loadSchema(const std::string& file);
    void loadData(const std::vector<std::string>& files);

    void addRelationship(const std::string& column,
                         const std::string& targetTable,
                         const std::string& targetColumn,
                         bool fuzzy = false);

    void processRelationships();

    template<typename Visitor>
    void accept(Visitor& visitor) {
        for (auto& record : _records) {
            visitor.visit(record);
        }
    }

    std::string toHTML();
    std::string toJSON();
    std::string toCSV();
};
```

## 13. Build System and Manufacturing

### 13.1 Image Builder

```cpp
class ImageBuilder : public Service {
private:
    BuildConfig _config;
    std::vector<Layer> _layers;
    std::vector<uint8_t> _imageData;
public:
    void setConfig(const BuildConfig& config);
    void addLayer(const Layer& layer);

    bool build();
    bool write(const std::string& outputPath);

    // Preset configurations
    static BuildConfig desktopWorkstation();
    static BuildConfig serverHeadless();
    static BuildConfig kioskTerminal();
    static BuildConfig embeddedIoT();
};
```

### 13.2 Build Configuration

```cpp
struct BuildConfig {
    // Identity
    std::string manufacturerId;
    std::string productId;
    std::string productName;
    std::string version;

    // Hardware
    std::string architecture;    // x86_64, arm64, riscv64
    int cpuCores;
    uint64_t memoryMB;
    uint64_t storageMB;

    // Software
    InterfaceType interface;     // desktop, kiosk, terminal, headless
    SecurityLevel security;      // none, standard, high, military
    OptimizationLevel optimization; // size, speed, balanced

    // Features
    std::vector<std::string> features;
    std::vector<ServiceConfig> services;
    std::vector<ApplicationConfig> applications;

    // Output
    std::string outputPath;
    std::string outputName;
    ImageFormat format;          // raw, iso, uefi, netboot
    bool compress;
    bool sign;
};
```

### 13.3 Manufacturing Workflow

```text
┌───────────────────────────────────────────────────────────────────────────
──┐
```

```text
│                        MANUFACTURING WORKFLOW
│
```

```text
├───────────────────────────────────────────────────────────────────────────
──┤
│                                                                              │
```

```text
│  1. Manufacturer Registration
│
```

```text
│     └── Submit application → Verify identity → Receive certificate
```

```text
│
```

```text
│                                                                              │
```

```text
│  2. Device Profile Creation
│
```

```text
│     └── Select motherboard → Configure CPU/RAM/Storage → Save profile
```

```text
│
```

```text
│                                                                              │
```

```text
│  3. OS Requirement Selection
│
```

```text
│     └── Choose interface → Select features → Pick services → Add apps
```

```text
│
```

```text
│                                                                              │
```

```text
│  4. Image Compilation
│
```

```text
│     └── Resolve dependencies → Build layers → Optimize → Sign
```

```text
│
```

```text
│                                                                              │
```

```text
│  5. Quality Assurance
│
```

```text
│     └── Verify checksum → Test in VM → Validate signatures
```

```text
│
```

```text
│                                                                              │
```

```text
│  6. Distribution
│
```

```text
│     └── Upload to CDN → Flash to devices → OTA update channel
```

```text
│
```

```text
│                                                                              │
└───────────────────────────────────────────────────────────────────────────
──┘
```

## 14. API Reference

### 14.1 Core API

| Function | Description | Example |
| --- | --- | --- |
| Box::Box(value) | Box b(42); | Create box from value |
| Box::as<T>() | int n = b.as<int>(); | Extract typed value |
| ObjectManager::registerObjec | objMgr.registerObject(myObj); | Register kernel object |

```text
t()
ObjectManager::resolve<T>()
(id);
Task::create()
Scheduler::addTask()
File::read()
Directory::lookup()
dir.lookup("config.txt");
```

### 14.2 JIT API

| Function | Description | Example |
| --- | --- | --- |
| JITCompiler::addStatement() | jit.addStatement(Stmt::A | Add statement to compile |
| JITCompiler::compile() | jit.compile(); | Generate machine code |
| JITCompiler::execute() | Box result = jit.execute | Run compiled code |
| Expr::BinOp() | Expr::BinOp(BinOp::ADD, | Create binary expression |
| Stmt::If() | Stmt::If(cond, thenBlock | Create conditional |

### 14.3 Database API

```text
Function             Description             Example
UniversalDataba      Execute query            auto stream = db.query("SELECT * FROM
se::query()                                   users");
IRecordStream::      Get next row             while (stream->next(row)) { ... }
```

```text
next()
UniversalDataba
se::begin()
UniversalDataba
se::commit()
```

### 14.4 Error Handling

NEBULA uses the  Result<T>  pattern instead of exceptions:

```cpp
template<typename T>
class Result {
private:
    std::optional<T> _value;
    std::optional<Error> _error;

public:
    Result(T value) : _value(value) {}
    Result(Error error) : _error(error) {}

    bool isOk() const { return _value.has_value(); }
    bool isError() const { return _error.has_value(); }

    T value() const { return _value.value(); }
    Error error() const { return _error.value(); }

    T operator*() const { return value(); }
};
```

## 15. Appendices

### Appendix A: System Calls

| Number | Name | Description |
| --- | --- | --- |
| sys_object_create | 0 | Create new object |
| sys_object_destroy | 1 | Destroy object |
| sys_object_get | 2 | Get object reference |
| sys_task_create | 3 | Create task |
| sys_task_yield | 4 | Yield execution |
| sys_task_sleep | 5 | Sleep for duration |
| sys_file_open | 6 | Open file |
| sys_file_read | 7 | Read from file |
| sys_file_write | 8 | Write to file |
| sys_file_close | 9 | Close file |
| sys_memory_alloc | 10 | Allocate memory |
| sys_memory_free | 11 | Free memory |
| sys_ipc_send | 12 | Send message |
| sys_ipc_recv | 13 | Receive message |

### Appendix B: File Types

| Extension | Type | Description |
| --- | --- | --- |
| .nebula | System image | Bootable OS image |
| .nebula-app | Application | Packaged application |
| .nebula-layer | Layer | System layer component |
| .nebula-config | Configuration | Build configuration |
| .nebula- | Manifest | Metadata file |

```cpp
manifest
```

### Appendix C: Environment Variables

| Variable | Description | Default |
| --- | --- | --- |
| NEBULA_HOME | /home/user | User home directory |
| NEBULA_PATH | /usr/bin:/usr/local/bin | Executable search path |
| NEBULA_CONFIG | /etc/nebula | Configuration directory |
| NEBULA_CACHE | /var/cache/nebula | Cache directory |
| NEBULA_LOG | info | Log level |

### Appendix D: Glossary

Term

Definition Box  Universal type container for

all data

Object  Base class for all kernel

```entities
Provision
Layer
```

```component
Image
Task
(thread/process)
Result
JIT
```

## Conclusion

NEBULA represents a complete rethinking of operating system design around object-oriented

principles. By unifying all system components under a single object model, providing a powerful

JIT compilation system, and offering flexible manufacturing pipelines, NEBULA delivers:

- Simplicity through uniform object interfaces
