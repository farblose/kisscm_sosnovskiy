# Практика 1
## Задание 1
Отвтет: cat etc/passwd | sort | cut -f1 -d ":"

![image](https://github.com/user-attachments/assets/1d057450-6d06-47eb-bb69-3238d47f88bd)
## Задание 2
Ответ: cat /etc/protocols | sort -r -k2 -n | awk '{print $1, $2}' | sed -n 1,5p

![image](https://github.com/user-attachments/assets/006fa9d0-56e2-41d0-8a26-e26db1b4aeff)