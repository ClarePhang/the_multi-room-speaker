# Multiroom Speaker System (Linux / TCP-IP)

##### Prerequisites
   * Gaming router
   * Server wired to the local network
   
##### Git content

   * The Server source code
   * The Client source code
   * An Android apk proof of concept build on NDK using QT5.12 
      
   
#### Install prerequisites on speaker (R-PI or any other ARM) (Linux x86 or ARM)

```
################################################
## Machine(s) 1: With alsa and speakers (client)
sudo apt-get install libao-dev 
sudo apt-get install libalsa-dev   <optional>
sudo apt-get install libmpg123-dev <optional>
sudo apt-get install libncurses*-dev <optional / or libGL libGLUT libGLU depending of the UI>
```

#### Make the client
```
#################################
git clone https://github.com/circinusX1/tcp_player
cd tcpspkcli
./make.sh
./tcpspkcli
```

#### Make the server
```
#################################
## Machine 2 (server)
sudo apt-get install libncurses*-dev
sudo apt-get install mplayer
git clone https://github.com/circinusX1/tcp_player
cd tcpspksrv
./make.sh
./tcpspksrv <client_side_sync>
```

#### Pipe some music to server pipe file

```

#################################
## Machine 2 (second terminal)
cd test
./sick.sh
./vacanttza.sh
```

#### Alsa on ARM

```
 nano /usr/share/alsa/alsa.conf
 # should be
pcm.default cards.pcm.default
pcm.sysdefault cards.pcm.default
pcm.front cards.pcm.default

 ```


   * Demo screen shot. Server uses ncurses lib
    
![screenshot](https://raw.githubusercontent.com/circinusX1/tcp_player/main/docs/scrshot.png)


   * OpenGL server console. Shows the PID speed BPS data that goes to the pilot client.
   
![screenshot](https://github.com/circinusX1/reference_images/blob/master/jump3.png)


