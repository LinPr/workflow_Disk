# workflow_Disk
This is a project of an online disk based on rpc framework, which is an open source project named sogo/workflow


由于http协议本身是无状态的协议，引入mysql将客户端的状态转移存储至数据库，提高效率
扩展：浏览器的cookies机制是将状态转移至客户端存储（由浏览器来完成这个工作）


互联网服务离不开用户认证。一般流程是下面这样。

1、用户向服务器发送用户名和密码。
2、服务器验证通过后，在当前对话（session）里面保存相关数据，比如用户角色、登录时间等等。
3、服务器向用户返回一个 session_id，写入用户的 Cookie。
4、用户随后的每一次请求，都会通过 Cookie，将 session_id 传回服务器。
5、服务器收到 session_id，找到前期保存的数据，由此得知用户的身份。