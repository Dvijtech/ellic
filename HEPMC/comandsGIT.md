# Перейти в корень репозитория
cd C:\Users\Dim\YandexDisk\ELLIC\ellic

# Получить последние изменения с GitHub
git pull origin main --rebase

# После перемещения проверить статус Git
git status
# Будут показаны удалённые файлы (из старых мест) и новые (в media)

# Добавить все изменения в индекс
git add -A

# Сделать коммит
git commit -m "Move all media files to media/ folder"

# Отправить на GitHub
git push origin main