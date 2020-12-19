#### TcpClient&TcpServer 说明

1. 用户A加入room，通知room中每个人“user_A join in room”。
2. 用户A离开room，通知room中每个人“user_A left room”。

![image-20201213220343291](typora-user-images/image-20201213220343291.png)

3. 用户A发送消息，其消息需要通知room中每个人。

![image-20201213220658273](typora-user-images/image-20201213220658273.png)

4. client功能说明

![image-20201213220859638](typora-user-images/image-20201213220859638.png)

5. server功能说明

![image-20201213220905952](typora-user-images/image-20201213220905952.png)