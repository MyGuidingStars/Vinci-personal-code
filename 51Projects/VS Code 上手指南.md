# 51 单片机：用 VS Code 代替 Keil 编辑器

编译器仍然是 Keil C51（`G:\Keil5\C51\BIN\C51.exe`），只是把难用的编辑器换成 VS Code。
**你的源代码一行都不用改。**

---

## 一、打开方式（最重要）

打开 VS Code → `文件` → `打开文件夹` → 选择：

```
C:\Users\lenovo\Desktop\Keil5
```

> 必须打开**文件夹**，不要只打开单个 .c 文件。
> 因为 `.vscode` 配置是放在这个文件夹里的，只开文件的话配置不会生效。

首次打开会提示"是否信任此文件夹的作者"，选**是**，否则代码片段和任务都不会工作。

---

## 二、编译

按 `Ctrl + Shift + B`，会编译**你当前编辑的文件所在的那个工程**。

比如你正在改 `2-2LED闪烁\main.c`，按下去就编译 `2-2LED闪烁\2-2LED闪烁.uvproj`，不用手动选工程。

其他任务：`Ctrl + Shift + P` → 输入 `运行任务` → 选择：

| 任务 | 作用 |
|---|---|
| ① 编译当前文件所在工程 | 最常用 |
| ② 全部重新编译（Rebuild） | 改了头文件、编译异常时用 |
| ③ 从列表选工程编译 | 想编译别的工程时 |
| ④ 清理编译产物（Clean） | 清空 Objects |
| 打开 STC-ISP 下载程序 | 直接启动烧录软件 |

**报错直接跳转**：编译出错时，`问题` 面板（`Ctrl + Shift + M`）会列出每条错误，
单击就跳到对应代码行。这比 Keil 的 Build Output 窗口好用得多。

---

## 三、烧录（和以前一样）

编译成功后，hex 文件在工程目录的 `Objects\` 里。

打开 STC-ISP → 选择单片机型号 `STC89C52RC` → 选择 hex 文件 → 点"下载/编程" → 给开发板断电再上电。

---

## 四、代码片段（提速重点）

在 `.c` 文件里输入下面的缩写，按 `Tab` 就会展开成完整代码：

| 输入 | 展开内容 |
|---|---|
| `main51` | 主函数框架（含 `#include <REGX52.H>`） |
| `delay` | 延时函数（@12MHz 初值） |
| `timer0` | 定时器0初始化（1ms @12MHz） |
| `isr0` | 定时器0中断服务函数 |
| `timer1` | 定时器1初始化 |
| `uart` | 串口初始化 + 发送函数 + 接收中断 |
| `exti0` | 外部中断0初始化 |
| `isrexti0` | 外部中断0服务函数 |
| `sbit` | `sbit LED = P2^0;` |
| `key` | 独立按键读取（带 20ms 消抖 + 松手检测） |
| `segtable` | 共阴数码管段码表 |
| `hd51` | 模块化编程用的 `.h` 头文件模板 |

---

## 五、常用快捷键（Keil 里没有的）

| 快捷键 | 作用 |
|---|---|
| `Alt + 单击` | 多光标，批量改端口号超好用 |
| `Ctrl + D` | 选中下一个相同的词，一起改 |
| `Ctrl + H` | 查找替换，可开正则（`.*` 图标） |
| `Ctrl + 单击` | 跳到函数/变量定义 |
| `Ctrl + P` | 按文件名快速打开 |
| `Ctrl + Shift + M` | 查看编译错误列表 |
| `Ctrl + /` | 注释/取消注释 |

---

## 六、常见问题

**中文注释乱码？**
项目已设置为 GBK 编码（Keil 的原生编码），正常不会乱码。
如果某个文件仍然乱码，点右下角的编码名称 → `通过编码重新打开` → 选 `GBK`。

**编译提示"检测到 Keil μVision 正在运行"？**
Keil 是单实例程序。这个只是提醒，实测即使 Keil 开着也能编译成功。
如果结果异常，把 Keil 窗口关掉再编译。

**想用 EIDE 插件的图形界面？**
已安装 EIDE 插件（左侧栏图标），Keil C51 路径也配好了（`G:\Keil5\UV4\UV4.exe`）。
侧边栏 EIDE → `导入项目` → 选 `Keil C51 Project` → 打开 `.uvproj` 即可。
注意导入时不要选 SDCC 工具链，要选 Keil C51。

**两个工程文件是 0 字节怎么办？**
`5-1模块化编程.uvproj` 和 `2-3流水LED\2-3流水LEDPlus.uvproj` 目前是空文件，
Keil 打不开。需要从其他工程复制一个改名后重新配置，或重新新建工程。

---

## 附：文件位置

| 内容 | 路径 |
|---|---|
| Keil uVision | `G:\Keil5\UV4\UV4.exe` |
| C51 编译器 | `G:\Keil5\C51\BIN\C51.exe` |
| C51 头文件 | `G:\Keil5\C51\INC`（`REGX52.H` 在 `INC\Atmel\`） |
| STC-ISP | `C:\Users\lenovo\Desktop\stc-isp.exe` |
| 本项目配置 | `C:\Users\lenovo\Desktop\Keil5\.vscode\` |
---

## 七、已知报错说明

### 1. IntelliSense 的红波浪线（sfr / sbit）

Keil 的寄存器头文件（`REGX52.H` 等）使用 `sfr`、`sbit` 这类 C51 专有关键字，
标准 C 解析器不认识，会让代码补全和错误检查整片失效，还会连带报出
莫名其妙的错误（例如 `delay` 被误判成声明冲突）。

已在 `.vscode\c_cpp_properties.json` 的 `defines` 里把它们映射成标准 C 类型：

    "sfr=unsigned char",
    "sbit=unsigned char",
    "bit=unsigned char",
    "data=", "idata=", "xdata=", "pdata=", "code=", "reentrant="

**这不影响实际编译** —— Keil 编译器的头文件搜索路径由 `.uvproj` 决定，
仍然读真正的 Keil 头文件，VS Code 这份映射只服务于补全和跳转。

改完配置需要重新加载窗口才会生效：

    Ctrl + Shift + P  →  输入「重新加载窗口」→ 回车

### 2. 中断函数仍有波浪线

    void Timer0_Routine() interrupt 1

`interrupt` 关键字没法映射成合法 C，所以这一行会被标红。
**这是正常的，不影响编译。** 如果觉得碍眼，在 `.vscode\settings.json` 里加一行：

    "C_Cpp.errorSquiggles": "disabled"

代价是真正的语法错误也不再提示了，自己权衡。

### 3. EIDE 插件报错 Install eide binaries failed

EIDE 需要从 GitHub 下载额外组件，国内网络通常连不上，所以它一直弹这个。

**本方案不依赖 EIDE**，编译走的是 Keil 命令行 + VS Code 任务，功能完全够用。
想彻底消除这个报错，在终端里执行：

    code --uninstall-extension CL.eide

（以后网络条件好了、又想要 EIDE 的图形界面，可以重新安装。）
---

## 八、一键新建实验工程

每个实验文件夹都必须有自己的 `.uvproj` 工程，否则按编译会往上找到别的工程，
出现「明明代码没错却报错」的情况（因为编的是别人的文件）。

新建实验不用再手动操作，两种方式任选：

**方式 A：VS Code 任务面板**

    Ctrl + Shift + P  →  运行任务  →  ⑤ 新建实验工程
    输入实验名，例如：5-2定时器中断

**方式 B：终端命令**

    cd C:\Users\lenovo\Desktop\Keil5
    powershell -ExecutionPolicy Bypass -File .\.vscode\new-project.ps1 -Name "5-2定时器中断"

脚本会自动完成四件事：

1. 创建 `5-2定时器中断\` 文件夹
2. 生成 `5-2定时器中断.c`（带 `#include <REGX52.H>` 和主循环框架）
3. 生成 `5-2定时器中断.uvproj`（器件 AT89C52、已勾选 Create HEX File）
4. 立即编译一次，确认能出 hex

可加参数：

| 参数 | 作用 |
|---|---|
| `-NoCompile` | 只建工程，不编译 |
| `-Root "D:\别的目录"` | 建到其他目录 |
| `-Template "路径"` | 用指定的工程当模板 |

> 如果目录里已有同名工程，脚本会拒绝执行，避免覆盖你的代码。

建好之后，打开那个 `.c` 文件按 `Ctrl + Shift + B` 就能编译出 hex。