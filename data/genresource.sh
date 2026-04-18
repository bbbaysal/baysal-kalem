#!/bin/sh
cd data
genresource(){
    echo "<RCC>"
    echo "    <qresource prefix=\"/\">"
    {
        echo "tr.org.pardus.pen.default.conf"
        echo "ikon.png"
        find images -type f
    } | sort | sed "s/^/        <file>/g;s/$/<\/file>/g"
    echo "    </qresource>"
    echo "</RCC>"
}

genresource > resources.qrc
