	windows自带的本地磁盘搜索极其难用，太慢了，慢的没有存在和使用的价值；后来
无意中发现了 Everything 这个本地磁盘搜索利器，非常之好用，速度飞快，占内存极小；用
后爱不释手，一般情况下，都是放后台运行，用的时候快捷键呼出，顺手省事；

	近来在做一个用户文件系统的内部搜索（即用户搜索自己的文件），和本地文件搜索
基本一个东西；只不过每个用户一个索引而已，希望搜索功能能达到Everything的效果即可，
同时因为用户量庞大，所以希望每用户文件索引占内存小；

	开始想用开源的玩意来做，但感觉有点牛刀杀鸡了，每个用户平均就万把数量文件，
应该有简单且傻瓜的方案完成这一功能，因为我发现 Everything 其实搜索挺傻的，没google
百度等搜索引擎那么智能（比如：如果搜索词“鼠标了”或者“鼠标的”就搜索不出“鼠标”
，就因为搜索词中多了一个无意义的词），搜索引擎很复杂，能分词，能区分出有意义无意义
的词，Everything 肯定用这些复杂的处理，但也很好用，够用；

	于是我在想怎么达到 Everything 的搜索效果，其实挺傻的一方案就是：把文件名直
接全部拆分成一个个的无任何字面意义的“字”，比如：
	鼠标        ->    鼠+标
	python大全  ->    python+大+全 或者更进一步 -> p+y+t+h+o+n+大+全
	为什么第二个需要更一步的拆分，因为发现 Everything 竟然能把 "p h" 匹配上 
python；

	因为用户的文件数量有限，加上常用“字”就那么些（常用汉字其实才几千），所以
最后每个用户文件系统所有文件名拆分得到的“字”肯定不会很多，然后把每个“字”对应的
文件这一映射建立起来，比如：
	假设某用户有两个文件a，b，文件名分别为“老鼠”，“鼠标”，最后的映射为：
	老          ->    a
	鼠          ->    a,b
	标	    ->    b
	
	映射建立完成后，实际上搜索就是求一个交集而已，比如搜索输入“老鼠”，拆分输入
后得到“老”和“鼠”，然后求这两个字所对应文件的交集，很简单最后得到 a

	原理很简单，实现过程中有几个问题：
	1，有些常用字比如数字1可能对应非常多的文件，所以存储内存占用会比较大
	2，求交集这一算法有可能会比较耗时，最快的交集算法是：两排序集合A,B交集算法复
杂度=(n(A)+n(B))，如果一个搜索输入拆分后的“字”数很多，求交集会更慢

	能不能解决这两个问题是决定这一方案能不能用的关键因素
	
	关键还是分析需求，关键点是：用户文件数量是有限的，假设一般为10万，最大不超过200
万（我C盘文件数不到9万），所以可以把文件从0开始映射到一个整数，这个整数肯定不会超过总
的文件数量；
	问题转化成：
	1，怎么保存一个元素大小为一定范围内的整数集合？
	2，怎么快速的求出这样两个集合的交集？

	先简化下：
	假设这个范围为[0-64)，最快最好的方法如下：
	这个集合完全可以用一个 uint64 来表示，这个集合的元素对应此整数的bit，实际上就是
把集合的元素映射到此整数的某一个bit上，比如：
	{0, 3, 4, 6} -> 1011001
	{0, 2, 4, 5} -> 0110101
	求量集合的交集，转化成了这两个整数的与操作，然后再逆映射回集合即可，具体如下：
	0010001      -> {0,4}

	于是求交集转化成了：
	1，与操作
	2，求结果整数对应的bit为1的位
	怎么快速的得到一个 uint64 的bit为1的所有位呢？我想最快的结果是将此 uint64 拆分成
四部分，每部分为一个 uint16 的整数，然后预先计算出所有的 uint16 对应的bit为1的位缓存好，
然后再合并即可

	进一步：
	范围超过了64怎么办？比如：[0, 64*64)，可以在上面的基础上延伸下：
最简单的办法是：64个 uint64 就可以表示一个集合了，但这样对于一个稀疏的集合却浪费极大，假
如此集合才4个元素，那至少有 60 个 uint64 为0都浪费了，甚至63个都为0（如集合={2,8,15,62}）
呃，如果我用另外一个额外的 uint64(叫root) 来表征这 64 个 uint64 怎么样？即：每个 uint64 
映射到root的每一bit，如果等于0，则对应bit置为0，否则为1；这样对于稀疏集合来说将节省大量的
uin64，比如 {2,8,67,71} 只需要3个 uint64 表达：
	root = 11, 子uin64分别为 100000100, 10001000

	求交集：先对root进行与操作，然后对bit为1的子uint64进行与操作即可

	再进一步：
	以此类推，如果范围为 [0, 64*64*64)，那么三层 uint64 就可以表达...

	呃，代码写起来其实挺麻烦，大量的内存操作和位操作，很容易搞错，三天搞定代码加测试
经过随机测试，最终结果，求交集速度是普通的“21倍”以上！！

	数据结构和算法已上传到git：https://github.com/kevin-zhong/bits_tree
	用法见测试代码：https://github.com/kevin-zhong/bits_tree/blob/master/bits_tree_testor.h
	












