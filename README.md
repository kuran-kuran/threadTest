# threadTest
## スレッドテストプログラム。  
スレッドのデータ通信、GPIOのハンドシェイクの部分をCreateEvent、SetEvent、ResetEvent、WaitForSingleObjectに置き換えて双方専用命令で待つようにしたらReleaseビルドでもデータ転送が成功するようになった。  

~~このプログラムは動作しません。~~  
~~バグっています。~~  
Roy. @GHF03373 さんに教えてもらってGPIOアクセスに排他処理を追加しました。  

![image](https://github.com/kuran-kuran/threadTest/assets/57883554/1178c517-eabf-4570-a1b3-58967574a63f)

デバッグでビルドして実行するとこのように途中で止まってしまいます。

![image](https://github.com/kuran-kuran/threadTest/assets/57883554/3a7024a8-1be2-408b-9828-88eff57bf86b)

リリースでビルドして実行するとエラーがたくさん出ます。
