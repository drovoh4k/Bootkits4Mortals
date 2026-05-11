# 🦑 Desarrollo de Bootkits para Mortales

<p align="center">
    <img src="Images/bootkit_cartografo.png" alt="ldd" width="500" />
</p>

> Si yo pude, tú también (probablemente)


## 📜 Sobre el proyecto

Este proyecto nace con el objetivo de desmitificar la complejidad inherente al desarrollo de bootkits. La información disponible sobre este tema suele estar fragmentada y ser excesivamente técnica, lo que crea una barrera de entrada para principiantes.

Este repositorio proporciona una guía introductoria y práctica que abarca desde los conceptos teóricos básicos hasta la creación de un bootkit funcional, sirviendo como un punto de partida claro y accesible para cualquier persona interesada en este mundo.


## 🛠️ Requisitos

Antes de compilar cualquier proyecto necesitas:

- **Máquina Virtual Windows** — se recomienda Windows 10/11 (ver Anexo A)
- **EDK2** con **Visual Studio** — entorno de compilación para aplicaciones UEFI (ver Anexo B)
- **Dependencias**: Git, Python 3.x, NASM

> Los Anexos A, B y C están incluidos en el [documento TFM](TFM/TFM%20-%20Desarrollo%20de%20bootkits%20para%20mortales.pdf).

## 🚀 Despliegue rápido

Una vez compilado el proyecto, el script [`Scripts/LoadAndRunBootkit.ps1`](Scripts/LoadAndRunBootkit.ps1) automatiza el despliegue en la máquina virtual:

1. Verifica privilegios de administrador (se auto-eleva si es necesario)
2. Monta la partición EFI del sistema
3. Copia todos los `.efi` desde `C:\Bootkit` a `\EFI\Boot\Bootkit` en la ESP
4. Desmonta la partición EFI
5. Reinicia el equipo

```powershell
# Uso con rutas por defecto
.\Scripts\LoadAndRunBootkit.ps1
```


## 📂 Estructura del Proyecto

```
Bootkits4Mortals/
├── Scripts/
│   └── LoadAndRunBootkit.ps1       # Despliegue automatizado en la ESP
└── TFM/                            # Código y documento del TFM de máster
    ├── 00_HelloWorld/
    ├── 01_LoadDriverNTFS/
    ├── 02_ExfiltrationHostsNTFS/
    └── TFM - Desarrollo de bootkits para mortales.pdf
```

### TFM

Progresión del trabajo de máster, desde una aplicación UEFI básica hasta un bootkit funcional con acceso al sistema de ficheros NTFS.

| Carpeta | Descripción |
|---|---|
| `00_HelloWorld` | Aplicación UEFI "Hello World". Punto de entrada al entorno EDK2 y su jerarquía de compilación. |
| `01_LoadDriverNTFS` | Carga un driver NTFS desde UEFI para leer y escribir en particiones Windows. |
| `02_ExfiltrationHostsNTFS` | Bootkit funcional que aprovecha el driver NTFS para leer y modificar ficheros clave del sistema. |


## 🛣️ Siguientes Pasos

Este proyecto sienta las bases, pero es solo la punta del iceberg. La evolución natural consiste en desarrollar un loader capaz de inyectar un rootkit interceptando la cadena de arranque de Windows (ver capítulo 5 del TFM).

Para quienes deseen profundizar, se recomienda estudiar:

- **[Abyss](https://github.com/TheMalwareGuardian/Abyss):** Framework de investigación sobre técnicas avanzadas de bootkits.
- **[Benthic](https://github.com/TheMalwareGuardian/Benthic):** Framework de investigación sobre técnicas avanzadas de rootkits.


## 📨 Contacto

Este proyecto representa la guía que me habría gustado tener y no encontré cuando empecé en este mundo. No te ahogarás en información; al contrario, he filtrado y ordenado solo lo esencial para que construyas tu primer bootkit. El objetivo es simple pero poderoso: llevarte de 0 a 1. Y como muchos sabemos, ese primer paso es a menudo el más difícil de todos.

Soy [![LinkedinLogo](https://i.sstatic.net/gVE0j.png) Llorenç](https://www.linkedin.com/in/llorenc-garcia-merino/), quien ha puesto toda su ilusión en este proyecto. Si tienes cualquier tipo de duda, comentario o propuesta de mejora, simplemente contáctame. Estaré más que dispuesto a dedicarte el tiempo necesario para profundizar en cualquier aspecto o ~crear el próximo gran bootkit~ iniciar un proyecto ético juntos.

