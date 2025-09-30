# Virtualização

https://drive.google.com/file/u/2/d/1mFT3dzkV6B9R6ESXWmT7OTOTsdo0UTun/view?usp=drive_link

### exploits

- falhas que são exploradas em um programa
- se eu instalar cada serviço em uma maq virtual separada, se um exploit acontece em um deles,
ele só afeta os q estão na máquina virtual

### pq cloud virtualiza tudo?

- se vc precisar rentar pra alguem que precisa de menos poder, 
vc aluga uma maq virtual e elas limitam a configuração do hardware da máquina pro plano que vc contratou
- ao invés de ter uma máquina física e criam uma config com base no seu plano e cappam uma máquina bem parruda

## Tipos de virtualização

### bosch: interpretador - facil de programar porem um lixo muito lento

### recompilação dinâmica
- pega em tempo de execucao o código de máquina de uma arquitetura pro codigo de máquina de outra arquitetura
- diferença de desempenho muito grande

### hipervisor tipo 1
- como um sistema operacional, um único programa executando no modo mais privilegiado
- dar suporte a múltiplas máquinas virtuais
- um dos mais famosos é o **Xen**
- grandes datacenters

### paravirtualização
- existe na academia
- fazer tanto o hospedeiro como o hospedado saberem quais eles são e conversarem entre si
- o sistema hospedeiro fornece uma interface de programação
- emulam chamadas que dependem de acesso privilegiado
- potencial para maior desempenho