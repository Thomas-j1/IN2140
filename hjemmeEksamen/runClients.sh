for i in {1..5}
do
    gnome-terminal --tab --title="client $i" -- bash -c "./upush_client 555$i 'PKT 0 REG name$i';"
done