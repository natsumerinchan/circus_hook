# CIRCUS引擎 hook补丁

## 实现功能
- 1、优先从AdvData\MES_CHS读取脚本
- 2、字体修改(黑体)
- 3、日志系统
- 4、标题修改
- 5、编码修改为GB2312

## 编码范围修改
编码范围需参考 https://www.iteye.com/blog/shera-409666 用x32dbg手动修改,
在主模块搜索常数A0，过滤cmp开头的命令

(al可能为dl,80可能为98)
```
(省略内容)
cmp al,80
(省略内容)
cmp al,A0
(省略内容)
cmp al,E0
(省略内容)
cmp al,FC
(省略内容)
```
改为
```
(省略内容)
cmp al,80
(省略内容)
cmp al,FE
(省略内容)
cmp al,E0
(省略内容)
cmp al,FE
(省略内容)
```

## WndProc地址获取方法
使用IDA搜索字符串`wndproc`，定位到lpfnWndProc，其后面跟着的offset sub_xxxxxx的xxxxxx就是WndProc地址

## 已测试游戏及其WndProc地址
- [【160226】【La'cryma】Kadenz fermata//Akkord:fortissimo](https://vndb.org/v14019) :`0x41EE30`
- [【080829】【CIRCUS】ホームメイド スイーツ](https://vndb.org/v383) :`0x408B80`

