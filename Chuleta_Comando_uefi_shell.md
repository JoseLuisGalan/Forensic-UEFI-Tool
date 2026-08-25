# Chuleta de Comandos — EFI Shell

> Añade `-b` a cualquier comando para paginar la salida (equivalente a `| more`).  
> Añade `-?` para ver la ayuda detallada de un comando concreto. Ejemplo: `dmpstore -?`

---

## Información y Ayuda

| Comando | Descripción |
|---|---|
| `help` / `?` | Lista todos los comandos disponibles |
| `help -b` | Idem con paginación |
| `ver` | Versión del firmware UEFI |
| `guid` | Todos los GUIDs registrados en el sistema |
| `err` | Errores del sistema y nivel de severidad |
| `smbiosview` | Información de la tabla SMBIOS |

---

## Sistema de Ficheros

| Comando | Descripción |
|---|---|
| `ls` | Lista el contenido de un directorio |
| `cd` | Cambia el directorio actual |
| `mkdir` | Crea un directorio |
| `cp` | Copia ficheros |
| `mv` | Mueve o renombra ficheros |
| `rm` | Elimina ficheros o directorios |
| `type` | Muestra el contenido de un fichero de texto |
| `edit` | Editor de ficheros ASCII y Unicode |
| `hexedit` | Editor hexadecimal |
| `attrib` | Muestra o cambia atributos de ficheros/directorios |
| `comp` | Compara dos ficheros byte a byte |
| `touch` | Actualiza el timestamp de un fichero |
| `vol` | Información del volumen (etiqueta, sistema de ficheros) |
| `mount` | Monta un sistema de ficheros o dispositivo de bloques |
| `eficompress` | Comprime un fichero con el algoritmo EFI estándar |
| `efidecompress` | Descomprime un fichero comprimido con `eficompress` |

---

## Dispositivos y Drivers

| Comando | Descripción |
|---|---|
| `devices` | Dispositivos gestionados por drivers EFI |
| `devtree` | Árbol de dispositivos del sistema |
| `drivers` | Lista de drivers EFI cargados |
| `dh` | Información detallada de los handles |
| `pci` | Configuración PCI de los dispositivos |
| `dblk` | Contenido de bloques de un dispositivo |
| `map` | Mapeado de dispositivos (`fs0:`, `blk0:`, etc.) |
| `connect` | Conecta drivers EFI a un dispositivo |
| `disconnect` | Desconecta drivers EFI de un dispositivo |
| `reconnect` | Reconecta drivers EFI a un dispositivo |
| `load` | Carga (y conecta) uno o más drivers EFI |
| `unload` | Descarga un driver EFI |
| `loadpcirom` | Carga la PCI Option ROM de un dispositivo |
| `drvcfg` | Protocolo de configuración de un driver |
| `drvdiag` | Protocolo de diagnóstico de un driver |
| `opeinfo` | Protocolos y agentes asociados a un handle |
| `sermode` | Configura parámetros de los puertos serie |

---

## Memoria

| Comando | Descripción |
|---|---|
| `memmap` | Mapa de memoria completo del sistema |
| `dmem` | Contenido de una región de memoria |
| `mem` | Contenido de memoria (similar a `dmem`) |
| `mm` | Muestra/modifica MEM, MMIO, I/O, PCI, PCIe |
| `memAllocationTest` | Prueba de asignación de memoria |
| `stall` | Detiene el procesador N microsegundos |

---

## NVRAM y Variables de Entorno

| Comando | Descripción |
|---|---|
| `dmpstore` | Muestra **todas** las variables NVRAM del sistema EFI |
| `set` | Muestra o modifica variables de entorno de la shell |
| `alias` | Crea, muestra o elimina alias de comandos |

---

## Red

| Comando | Descripción |
|---|---|
| `ping` | Envía paquetes ICMP a una IP |
| `ifconfig` | Configuración IP de las interfaces de red |
| `telnetmgnmt` | Cambia el tipo de terminal de gestión remota |

---

## Consola y Control de Flujo (scripts `.nsh`)

| Comando | Descripción |
|---|---|
| `echo` | Imprime texto en consola |
| `cls` | Limpia la pantalla / cambia color de salida |
| `mode` | Muestra o cambia modo/resolución de consola |
| `pause` | Muestra mensaje y espera pulsación de teclado |
| `if` | Ejecución condicional |
| `for` | Bucle sobre un conjunto de elementos |
| `goto` | Salta a una etiqueta en el script |
| `shift` | Desplaza parámetros posicionales |
| `exit` | Sale de la shell o del script en ejecución |
| `reset` | Reinicia el sistema (frío o caliente) |
| `date` | Muestra o establece la fecha del sistema |
| `time` | Muestra o establece la hora del sistema |
| `timezone` | Muestra o configura el huso horario |

---

## Ayuda Detallada de Cada Comando

```
<comando> -?        Ayuda básica del comando
<comando> -? -b     Ayuda paginada (para salidas largas)
```

**Ejemplos:**

```
dmpstore -?
memmap -? -b
dh -?
```
