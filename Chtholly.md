## Chtholly 珂朵莉：系统编程的新平衡

Chtholly 是一门**编译型、通用**的系统编程语言。它旨在为开发者提供一个**兼具 C++ 性能和 Rust 内存安全**的优雅环境。

Chtholly 遵循**零成本抽象**以及**运行时极简**原则，尽可能把事情交给编译期进行，实现：
1.  **默认安全 (Safety by Default)：** 采用所有权与借用模型，在编译期根除内存错误。
2.  **人体工程学 (Ergonomics)：** 采用经典的类/结构体语法和智能的生命周期推导，降低系统编程的学习成本。

Chtholly文件后缀为`.cns`。

### 注释
```chtholly
// 单行注释

/*
多行注释
*/
````

### 主函数

```chtholly
fn main(args: std::string[]): Result<i32, std::string>
{

}
```

### 资源管理 (Resource Management)

Chtholly采用现代**所有权**和**借用**模型。

  * **所有权 (Ownership)：** Chtholly 中的每个值都有一个**所有者**。在任何时间点，一个值只能有一个所有者。当所有者超出作用域时，该值将被自动清理（Drop/析构），保证了内存安全。
  * **零成本抽象：** 所有权机制在编译期强制执行，运行时开销极低，无需垃圾回收（GC）。

### 不可变变量 (Immutable Variables)

Chtholly使用`let`声明不可变变量。

Chtholly编译器会自动推导变量的类型。

Chtholly支持类型注解。

  * **不可变性优先：** 默认情况下，所有绑定都是不可变的。`let` 关键字创建的变量，其绑定的**资源内容**在创建后不能通过该变量名被修改。

```chtholly
let a = 10;
let b = 30.4;
let c = 'a';
let d = "HelloWorld";
let e = [1, 2, 3, 4];

let a2: i32 = 10;
let b2: f64 = 30.4;
let c2: char = 'a';
let d2: std::string = "HelloWorld";
let e2: i32[] = [1, 2, 3, 4];
```

### 可变变量 (Mutable Variables)

你可以使用`mut`修饰不可变变量，使其可变。

  * **可变性：** `mut` 关键字允许所有者通过该变量**操作或修改**它所拥有的资源。可变性是作用于当前绑定的**权限**。

```chtholly
let mut a = 10;
```

### 变量间的传递：移动语义 (Move Semantics)

变量间以**移动**进行传递。`let`和`let mut`声明的变量相互之间可以自由赋值，这是因为变量相互之间的传递属于**资源转移**。

`let`和`let mut`仅决定指向的资源是否可以被操作。

  * **资源移动：** 当所有权发生转移（例如赋值给新变量、作为参数传递给函数）时，资源的所有权从原变量转移给新变量。**原变量在移动后立即失效且不可用**，防止了资源被二次释放。
  * **零成本复制 (`Copy` Trait)：** 对于基本数据类型（如整数、浮点数、字符等），赋值操作不是移动，而是**零成本的按位复制**。在这种情况下，**原变量在赋值后仍然有效**。


```chtholly
let a = 10;
a = 10;  // 错误，不可变变量无法操控这部分资源
let mut a2 = a; // 对于 Copy 类型 10，这里是复制，a 仍然可用。
a2 = 20;  // 正确，可变变量可以操控这部分资源
```

### 引用语义：借用 (Borrowing Semantics)

如果你想要同时共享一份资源，你可以使用引用符号`&`创建一个引用。

  * **借用：** 引用 (`&`, `&mut`) 允许在不转移所有权的情况下**临时访问**资源。借用必须遵守**生命周期**规则，生命周期用于**确保所有引用都比它们指向的数据存活的时间短**，从而防止悬垂引用。
  * 使用`&`创建**不可变引用（共享引用）**。
  * 使用`&mut`创建**可变引用（独占引用）**。

**借用规则（核心原则：读写互斥）：**

在任何时间点，对于同一份资源，以下两种借用情况只能存在一种：

  * **任意数量的共享引用 (&)：** 允许多个共享引用同时存在，但它们都不能修改资源。

  * **仅一个可变引用 (&mut)：** 允许通过该引用修改资源，但它必须是独占的。

  * 在某一作用域内，如果存在一个可变引用，则不能存在任何其他引用（无论是共享引用还是另一个可变引用）。这是 Chtholly 保证**数据竞争安全**的关键。

```chtholly
let a = 10;
let b = &a;  // 对a的资源的不可变引用

// 以下示例展示借用规则：
let mut x = 10;
let r1 = &x;    // 共享引用 R1
// let m1 = &mut x; // 错误：在 R1 存在时，不能创建可变引用 M1

// r1 超出作用域或不再使用...

let m2 = &mut x; // 正确：现在可以创建可变引用 M2
// let r2 = &x;     // 错误：在 M2 存在时，不能创建共享引用 R2
```

### 生命周期

生命周期是借用机制的核心，它用于确保所有引用都比它们指向的数据存活的时间短，从而防止悬垂引用。

```Chtholly
'a，'static a
```

#### 智能生命周期省略

Chtholly引入了智能的生命周期省略规则，旨在在 90% 的情况下消除手动生命周期注解，最大程度地减轻心智负担，同时保留 Rust 级别的编译期安全。

### 数据类型 (Data Types)

Chtholly有如下内置数据类型：

  * `i32`: 默认使用的整数类型。
  * 精细划分的整数类型：`i8, i16, i32, i64` (有符号)；`u8, u16, u32, u64` (无符号)。
  * `char`: 字符类型。
  * `f64`: 默认使用的浮点数类型。
  * 精细划分的浮点数类型：`f32, f64`。  
  * `void`: 表示空类型，通常用于函数不返回任何值的情况。
  * `bool`: 布尔类型。
  * 数组：`i32[]` (动态数组，所有权在堆上)，`i32[4]` (静态数组，大小编译期确定，通常在栈上)。
  * 函数：`function`，使用`(i32, i32): void`进行表示。

这部分的类型注解使用定义时的标识符：

  * `class`
  * `struct`

  * `string`: 表示动态字符串，默认使用 **UTF-8 编码**。(不是内置类型，标准库提供)

### 类型转换

Chtholly倾向于使用显式的 as 关键字进行原始类型之间的强制转换，以替代冗长的函数调用（如 type_cast<T>()）。

```chtholly
let float_val = 10.5;
let a: i8 = float_val as i8; // 结果为 10
```

### 溢出处理

所有溢出行为皆进行回绕。

### 运算符

Chtholly支持编程语言中的标准运算符。

```Chtholly
+ - * / %

== != > >= < <= !

& | ^ ~ << >>

= += -= *= /= %=

&& ||

++ --

exp ? :  // 条件表达式
```

### 函数 (function)

Chtholly使用`fn`关键字定义函数。

```Chtholly
fn hello(): void
{
    println("HelloWorld");
}

hello();

let test: (): void = hello;
```

#### 默认值

Chtholly支持函数参数具有默认值。

```Chtholly
fn hello(a: i32 = 20)
{

}
```

#### 函数重载

Chtholly支持函数重载。

```Chtholly
fn hello(a: i32)

fn hello(a: std::string)
```

#### 命名参数

```Chtholly
fn hello(a: i32)

hello(a: 10)
```

### lambda

Chtholly支持lambda表达式。

[]可以捕获外部变量，遵循a所有权，&a不可变捕获，&mut可变捕获的语义  

```Chtholly
let test = [](): void {
    println("HelloWorld");
}();
```

### 类 (Class)

你可以使用`class`关键字定义类。

#### 成员变量

Chtholly使用`let`声明不可变成员变量。

Chtholly使用`let mut`声明可变成员变量。

```Chtholly
class Person
{
    let name: std::string = "yhlight";  // 不可变成员变量，可以赋予默认值
    let age: i32;

    let mut des: std::string;  // 可变成员变量，也可以赋予默认值
}
```

#### 构造函数

Chtholly使用C++语法的构造函数  

```Chtholly
class Person
{
    let name: std::string = "yhlight";  // 不可变成员变量，可以赋予默认值
    let age: i32;

    let mut des: std::string;  // 可变成员变量，也可以赋予默认值

    Person(age: i32, des: std::string)
    {
        self.age = age;
        self.des = des;
    }
}

let test = Test();  // 空初始化，遵循零初始化
let test2 = Test(18, "yhlight");  // 构造函数初始化
let test3 = Test{"yhlight", 18, "yinghuolight"};  // 聚合初始化(注意，聚合初始化会覆盖默认值)
let test4 = Test{des: "yhlight", age: 18};  // 指定初始化(注意，指定初始化会覆盖默认值)
```

#### 析构函数

在必要的时候，Chtholly可以使用析构函数管理资源释放后的行为。

析构函数将在所有权失效后执行调用。

从底层来看，析构函数本质是一个专门用于资源释放的函数。

```Chtholly
class fileSysteam
{
    ~fileSysteam()
    {

    }
}
```

#### 成员函数与Self

在Chtholly之中，使用Self表示本身。

你可以使用`fn`定义成员函数。

成员函数的第一位函数参数必须是所有权语义，即self(所有权)，&self(不可变)，&mut self(可可变)。

谨慎使用self，self会导致实例访问后所有权失效。

如果没有所有权语义，则表示与对象无关，此时需要使用::来访问。

使用Self表示本身。

```Chtholly
class Person
{
    let name: std::string = "yhlight";  // 不可变成员变量，可以赋予默认值
    let age: i32;

    let mut des: srting;  // 可变成员变量，也可以赋予默认值

    Person(age: i32, des: std::string)
    {
        self.age = age;
        self.des = des;
    }

    fn show(self): void
    {
        println(self.name, self.age, self.des);
    }

    fn print(): void
    {
        println("人");
    }

    fn add(self, other: Person): Self
    {
        return Self(self.age + other.age, std::combine(self.des, other.des));  // 返回对象本身
    }
}

let p = Person();
p.show();
Person::print();
```

#### 权限修饰

Chtholly默认私有，你可以使用pub修饰成员变量与成员函数，使其公开。

```Chtholly
class Person
{
    pub let name: std::string = "yhlight";
    pub let age: i32;
    pub let mut des: std::string;

    pub Person()
    {

    }

    pub fn show(self): void
    {

    }

    pub fn print(): void
    {

    }
}
```

#### 构造函数与self

对于构造函数与析构函数来说，它们不需要显性写出self，它们是一种特殊的函数，由编译器特殊处理。

但是对于构造函数与析构函数，它们仍需要使用self.来引用成员。

#### 继承

只支持组合式继承，请自行内嵌其他类对象。

#### 多态

暂不支持多态。

### 结构体 (Struct)

Chtholly使用`struct`关键字定义结构体。

struct不需要权限修饰符，默认公开。

结构体与类十分相似，但是struct是纯粹的数据集。

即struct不支持权限修饰符，不支持构造函数。

但是struct支持成员函数。

结构体不支持零初始化，这意味着你必须要保证结构体所有成员都得到了有效的初始化。

```Chtholly
struct Person
{
    let name: std::string;  // 支持默认值
    let age: i32;
    let mut des: std::string;

    fn show(self): void
    {
        println(self.name, self.age, self.des);
    }
}

let test = Test{"yhlight", 18, "des"};  // 聚合初始化(注意，聚合初始化会覆盖默认值)
let test2 = Test{name: "yhlight", id: 18};  // 指定初始化(注意，指定初始化会覆盖默认值)
```

### 枚举

Chtholly使用enum创建枚举。

Chtholly的枚举支持状态，即它们能够存储信息。

Chtholly要求所有的枚举都由switch-case进行匹配。

枚举只支持拷贝 / 移动，不支持&string和&mut string等语义。

```Chtholly
enum color<T>
{
    red, // 常规
    green,
    RGB(std::string, std::string, std::string),  // 元组
    RGB {  // 结构
        x: std::string;
        y: std::string;
        z: std::string;
    }
}

let a: color = color::red;
```

#### 枚举元组变体

在枚举之中，你可以使用(type, type, type)创建一个元组变体。

不支持&string和&mut string等语义

```Chtholly
enum color
{
    RGB(std::string, std::string, std::string)
}

let a = color::RGB("255", "255", "255");
```

#### 枚举结构

枚举结构不支持let，let mut的修饰，也不支持&string和&mut string等语义。

```Chtholly
enum color
{
    rgb {
        x: std::string;
        y: std::string;
        z: std::string;
    }
}
```

#### 枚举方法

枚举是一种不具有实例化语义的结构，不支持self语义，但是支持Self。

```Chtholly
enum Color
{
    fn test()
    {

    }
}
```

### 数组

Chtholly使用类型 + []创建数组。

i32[]为动态数组，i32[4]为静态数组。

a[0] = 10;这一种访问方式属于安全的静态访问。

```Chtholly
let a: i32[] = [1, 2, 3];
a[0] = 10;
```

### 选择结构
#### if-else

Chtholly使用C风格的if-else结构。

其中表达式的括号可以省略。

```Chtholly
if (a > b)
{
    print("a > b");
}
else if (a < b)
{
    print("a < b");
}
else
{
    print("a == b");
}
```

#### switch-case

Chtholly使用C风格的switch-case结构。

其中表达式的括号可以省略。

不一样的是，Chtholly的switch-case修复了C语言的switch-case的bug。

```Chtholly
switch(任意类型的变量 / 表达式)
{
    case 值1: 
        break;  // break现在不是防止穿透，而是跳出匹配
    
    case 表达式: 
        break;
    
    case 表达式2: 
        fallthrough;  // 如果需要穿透，请使用fallthrough

    default:
}
```

#### switch模式匹配

Chtholly现阶段暂时不支持除了switch-case以外的模式匹配。

我觉得也不需要提供其他的语法。

但由于switch-case天生具有单一击中的匹配能力，这意味着Chtholly实际上并不需要if-let语法。

要注意，switch-case的解构会默认转移所有权，如果需要避免所有权转移请使用&x或&mut x。

```Chtholly
enum color<T>
{
    red,
    green,
    RGB(std::string, std::string, std::string),
}

let a: color = color::red;

switch(a)
{
    case color<T>::red: 

    case color<T>::RGB(x, y, z):   // x, y, z就是自动解构的变量
}
```

#### switch进阶匹配

```Chtholly
switch(a)
{
    var => exp
    
    var2 => {

    }

    var3, var4 => {

    }

    _ => {
        
    }
}
```

### 循环结构
#### while循环

Chtholly使用标准C风格while循环。

其中表达式的括号可以省略。

```Chtholly
while (条件) {
    continue / break;
}
```

#### for循环

Chtholly使用标准C风格for循环。

其中表达式的括号可以省略。

```Chtholly
for (let i = 0; i < 10; i++) {
    continue / break;
}
```

#### do-while循环

Chtholly使用标准C风格do-while循环。

其中表达式的括号可以省略。

```Chtholly
do
{

} while (condition);
```

### 泛型

注意！Chtholly明确使用<T>而不是::<T>。

尽管这会很复杂，但Chtholly允许增加编译时间以换取简洁的泛型功能(实际上这会大大增加编译器负担，可能考虑放弃)。

Chtholly提供了另一种可选项，如果add<i32>()被证实难以实现，或者会带来许多问题。

那么Chtholly可以选择更换为add[i32](); 即将<>更换为[]，这一点在现代语言之中通常是高度支持。

值得一提，Chtholly的泛型更偏向于C++，而不是Rust，例如下述操作应该被允许，这也是后续约束实现类似Rust的关联类型的关键，在这里我要评论一下Rust的关联类型，这个语法有一些突兀，我觉得Chtholly不应该支持。  

```Chtholly
fn add<T>(a: &T, b: &mut T): T
{

}
```

#### 泛型函数
```Chtholly
fn add<T>(a: T, b: T): T
{
    return a + b;
}

add<i32>(1, 2);

fn add<T = f64>(a: T, b: T): T  // 默认值
{
    return a + b;
}

fn add<std::string>(a: std::string, b: std::string): string  // 特例化
{
    return std::concat(a, b);
}
```

#### 泛型类
```Chtholly
class Point<T>
{
    x: T;
    y: T;

    fn swap(self, other: Point<T>): Self
    {
        
    }

    fn swap(self, other: Point<T = i32>): Self
    {

    }

    fn swap(self, other: Point<i32>): Self
    {

    }
}

class Point<T = i32>
{

}

class Point<i32>
{

}

let p = Point<i32>(1, 2);
```

#### 类内的泛型函数

在Chtholly之中，类，无论自身是否是泛型，都可以拥有泛型成员函数。

##### 常规类内的泛型函数
```Chtholly
class Printer
{
    fn print<T>(self, value: T)
    {
        print(value);
    }
}

fn main()
{
    let p = Printer();
    p.print<i32>(10);  // 调用时指定类型
    p.print("hello");  // 或者让编译器自动推断类型
}
```

##### 泛型类内的泛型函数

泛型类内部也可以创建独立的泛型函数。

```Chtholly
class Point<T>
{
  // 方法的泛型参数 K, F 与类的泛型参数 T 是独立的
    fn test<K, F>()
    {

    }

    fn test2<K = i32, F>()
    {

    }

    fn test2<K, F = i32>()
    {

    }

    fn test2<K = i32, F = i32>()
    {

    }

    fn test2<i32, i32>()
    {

    }

}
```

### optional

Chtholly具有安全的optional类型。

optional需要optional模块的支持。

optional类型有两个主要方法，unwrap和unwarp_or。

```
let a = optional<i32>(10);
```

### 模块与import

Chtholly 支持模块系统，允许您将代码组织到多个文件中或使用标准库功能，`import`关键字用于加载另一个模块中的代码并将其合并到当前作用域。

#### 语法

`import` 语句接受两种形式：  
1.  **文件路径**: 一个字符串字面量，表示您想要包含的 Chtholly 文件（`.cns`）的路径。
2.  **标准库模块名**: 一个标识符，表示您想要导入的标准库模块。

import应该被用于导入模块，如果你需要导入模块中需要的成员，请使用use。

use语句同样支持别名。

路径默认是相对路径，相对当前文件。

```Chtholly
// 导入文件模块
import "path/to/your_module.cns";

// 导入标准库模块
import iostream;
```

#### 行为

你需要使用模块名::方法名来访问模块中的方法。

```Chtholly
import iostream;
use iostream::println as pl;  // 把这个函数导入到当前作用域

fn main()
{
    iostream::print("Hello, World!");
    println("Hello, World!");
}
```

#### 示例

假设您有一个名为 `math.cns` 的文件：

```Chtholly
// math.cns
pub fn add(a: i32, b: i32): i32
{
    return a + b;
}
```

您可以在另一个文件 `main.cns` 中通过导入 `math.cns` 来使用 `add` 函数：

```Chtholly
// main.cns
import "math.cns";

fn main()
{
    let result = math::add(5, 10);
    print(result); // 将输出 15
}
```

#### 冲突防止

通常情况下，Chtholly默认使用文件名作为模块名。
在冲突时，你可以为模块指定一个别名。

```Chtholly
import "math.cns" as math;
import "math.cns" as math2;
```

### 创建模块的最佳实践
我们建议模块使用对象式而不是函数式  

### 包
模块被收录在哪一个包？  

模块与物理结构的目录存在关联。

例如package std;那么此模块将会被存放在std文件夹之中。

此时任意放在std文件夹且具有package std;顶层语句的.cns文件，都将作为一个模块。

例如vec.cns，你可以在其他文件之中使用import std::vec;

这会导入vec.cns文件之中的所有内容。

要注意import std::vec时，vec会被隐去，因为它是文件名，此时你需要通过std::xxx来访问vec包中的内容。

此时你可以直接使用vec.cns文件之中的顶层定义。

也就是包作用域扁平化。

实际上是隐去模块名，也就是import std::vec; 意思就是导入std包下的vec.cns文件，但是vec.cns文件是一个无关的干扰项，

此时会隐去vec文件名，将作用域提升为std  

在冲突时，你需要写出完全的包名(带文件名)。

一般是std::vec::xxx的形式。

```nota
// 例如这个是数学模块
package packageName;

pub fn add(a: i32, b: i32): i32
{
    return a + b;
}

/////////////////////
import packageName::math;
import packageName2::math;

packageName::math::add(1, 2);  // 冲突时需要写出包名
```

### 导入，模块，包细则

导入有两种方式，即路径导入与模块导入。

对于路径导入，即"xxx/xxx/xxx.cns"，这一种行为不遵循文件层级，即不遵循package，命名空间就是文件名本身，使用文件名::进行访问。

对于模块导入，即std::io::iostream，这一种导入遵循package，即遵循文件层级，命名空间是std::io，最后一个是模块本身(文件名)，会被隐去，只有发生冲突时，才需要使用std::io::iostream进行访问，其他时间使用std::io访问即可。

### 错误处理 (Error Handling)

Chtholly采用 Result<T, E> 类型进行基于值的错误处理，以实现编译期强制检查和零成本抽象。

#### Result 类型

Result<T, E> 是一个枚举，用于封装可能成功或失败的操作结果：

Result属于内置模块，不需要导入。

```chtholly
enum Result<T, E>
{
    Ok(T),    // 成功，包含一个值 T
    Err(E)    // 失败，包含一个错误 E
}
```

#### 错误传播：? 操作符

为了简洁地在函数间传递错误，Chtholly 提供了 ? 操作符。该操作符只能用于返回 Result 类型的函数内部：

1.  如果 Result 是 Ok(T)，解包出值 T。
2.  如果 Result 是 Err(E)，立即将错误 E 从当前函数返回，实现错误传播。

```chtholly
fn process_file(path: std::string): Result<i32, i32>
{
    // 如果 file::open 失败，函数立即返回 Err(ErrorType)
    let handle = file::open(path)?; 
    
    // ... 使用 handle ...
    return Result<i32, i32>::Ok(1); 
}
```

#### 错误处理：模式匹配

对于复杂的错误处理，建议使用 `switch` 结构对 `Result<T, E>` 进行模式匹配：

```chtholly
let operation_result = read_and_process("test.cns");

switch (operation_result)
{
    case Result<i32, i32>::Ok(data):
        println("处理成功，数据为:", data);
        break;
    case Result<i32, i32>::Err(error):
        println("处理失败，错误信息:", error.message);
        break;
}
```

### 裸指针

#### 堆分配

Chtholly开发者在unsafe块之中操作裸指针。

开发者需要使用malloc函数分配内存以获得指针。

裸指针可以使用free语句立即释放内存。

属于内置函数。

```chtholly
unsafe {
    let ptr: i32* = malloc<i32>();
    free (ptr);
}
```

malloc支持分配i8, i16, i32, i64, char, f32, f64等基本数据类型。

也支持分配struct(结构体)，class(类)，((i32, i32): void)(函数)等复杂数据类型。

不支持enum(枚举)，i32[](数组)等特殊类型。

不支持分配u8, u16, u32, u64等有语义的类型。

### 栈分配

malloc主要用于堆分配，alloca则用于栈分配。

栈分配的内存不能free，其中windows为2MB，linux为8MB。

alloca函数提供一个参数，用于指定分配的空间的大小。

allocoa无法像malloc函数一样连续分配内存。

属于内置函数。

```chtholly
unsafe {
    let ptr: i32* = alloca<i32>();
}
```

### 内存布局与辅助函数 (Memory Layout & Helpers)

Chtholly 提供精确的内存控制工具，类似于 LLVM IR 的底层描述。

#### 内存辅助函数
* `sizeof(T)`: 获取类型 `T` 的占用空间（字节）。
* `alignof(T)`: 获取类型 `T` 的对齐要求。
* `offsetof(T, member)`: 获取结构体或类 `T` 中成员 `member` 的字节偏移量。

属于内置函数。

#### 成员布局控制
在 `struct` 或 `class` 定义中，可以对单个成员使用 `align` 和 `packed` 关键字。

* `align %Type`: 强制该成员按照 `%Type` 的对齐标准进行对齐。
* `align N`: 强制该成员按照 `N` 字节对齐。
* `packed`: 将该成员对齐要求设为 1（无填充缩紧）。

```chtholly
struct s {
    // x 占用 8 字节，但强制按照 i32 (4字节) 对齐
    let x: i64 align %i32 = 10;
    // y 紧随其后，不留任何填充 (Alignment = 1)
    let y: i32 packed;
}
```

#### FFI (External Function Interface)
```chtholly
extern fn printf(fmt: i8*, ...): i32;

unsafe {
    printf("Pointer: %p\n", nullptr);
}
```

### typedef

你可以使用typedef对一个已经存在的类型进行重命名。

```chtholly
typedef int = i32;
let a: int = 10;

typedef p = Point;
let b: p = p();
let b2: &p = &b;
```

### 约束

约束系统是Chtholly的核心之一，Chtholly采用的约束形式是分布式。

约束主要分为两种，一种是函数参数约束，一种是结构约束。

你可以使用request创建一个约束，使用require接入约束。

主要形式是结构约束。

结构约束目前仅支持class和enum。

```chtholly
request class logger
{
    pub let info: std::string = "logger";  // 权限修饰，是否可变，默认值

    // 默认情况下，所有接入logger约束的，都需要拥有这一个变量，这个变量不会默认提供
    
    default let source;  // 默认提供

    fn print_info(&self);  // 需要实现，可以拥有代码体，代码体仅提供模板作用，IDE应该直接提供这部分代码体给用户作为模板进行参考，无论是否具有代码体，都需要进行重写

    fn print_ver();  // 静态函数也一样不强制要求是否具有代码体

    default fn test();  // 默认提供，必须拥有代码体，可以重写
}

class log require logger
{
    fn print_info(&self)  // 重写
    {

    }
}

class log2 require logger, ...
{
    fn print_info(&self)  // 重写
    {

    }
}
```

#### 类约束

```Chtholly
request class logger
{
    pub let info: std::string = "logger";  // 权限修饰，是否可变，默认值

    // 默认情况下，所有接入logger约束的，都需要拥有这一个变量，这个变量不会默认提供
    
    default let source;  // 默认提供

    let x: i32 align %i64 = 10;  // 同样，align，packed这些成员的属性也可以在约束之中被使用

    fn print_info(&self);  // 需要实现，可以拥有代码体，代码体仅提供模板作用，IDE应该直接提供这部分代码体给用户作为模板进行参考，无论是否具有代码体，都需要进行重写

    fn print_ver();  // 静态函数也一样不强制要求是否具有代码体

    default fn test() {}  // 默认提供，必须拥有代码体，可以重写
}
```

#### 枚举约束

```Chtholly
request enum logger
{
    info(std::srting),  // 默认情况下，所有接入logger约束的，都需要拥有这一个变体，这个变体不会默认提供

    default debug_info {  // 默认提供
        err: std::string
    }

    fn print_info();  // 需要实现，可以拥有代码体，代码体仅提供模板作用，IDE应该直接提供这部分代码体给用户作为模板进行参考，无论是否具有代码体，都需要进行重写

    default fn test() {}  // 默认提供，必须拥有代码体，可以重写
}
```

#### 泛型约束

约束可以定义泛型参数，且实现接收必须具有同等 / 大于的泛型参数数量才能接入此约束。

```Chtholly
request enum logger[K]  // 名称随意，接入时需要手动指定
{

}

class log2[T] require logger[T], ...
{
    
}
```

#### 约束的继承

```Chtholly
request class logger
{
}

request class logger2 : logger
{
}

request class logger3 : logger, logger2
{
}
```

#### 多约束

无论是函数参数，还是结构，都可以接入多个约束，可以通过满足或满足其中一个约束。

```Chtholly
fn add[T ? std::is_integer || std::is_float](x: T, y: T)  // 或
{

}

fn add[T ? std::is_integer && i32](x: T, y: T)  // 并
{

}

class debuger require logger, info, ...
{
    // 发生冲突请使用logger::xxx，info::xxx明确区分
}
```

#### 函数参数约束

函数参数约束用途很少，一般无法自定义，多数由编译器提供。

##### 类型约束

类型约束主要用于判断参数的类型是否是某一个具体的类型(用于泛型)。

或者是判断参数类型是否实现了某一个约束。

或者是判断参数的类型是否符合某一类类型(编译器内置)。

```Chtholly
fn add[T ? i32](x: T, y: T)
{

}

request class logger
{
    
}

fn print_log[T ? logger](log: T)
{

}
```
