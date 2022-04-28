for i in {1..5}
do
    gnome-terminal --tab --title="client $i" -- bash -c "./upush_client name$i 127.0.0.1 4321 3 10;"
done