# tp-2018-2c-Nene-Malloc

### Orden de ejecucion de los procesos
* MDJ se queda escuchando el socket hasta que se conecte DAM
* DAM termina el handshake con MDJ y queda escuchando que se conecte CPU  
(Por ahora lo hace en ese orden, deberian ser hilos distintos y tiene que admitir n CPUs.)
* CPU inicia e intenta conectar a DAM por lo que falla si DAM todavia no completo el primer handshake  

### Tmuxp para poder manejar sesiones de tmux - [The Tao of tmux](https://leanpub.com/the-tao-of-tmux/read)

* Instalar tmuxp (necesita pip)
```bash
sudo apt install python-pip
sudo pip install tmuxp
```
* El .json usa sleep para simular un poco de espera para que se levanten en orden
```bash
tmuxp load SO.json
```
