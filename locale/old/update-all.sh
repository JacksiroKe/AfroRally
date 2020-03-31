#!/bin/bash -e

echo "Generating new template..."
`which python2` ./xml_po_parser.py ../data/gui/core_language_en_tag.xml ./stuntrally.pot

if [ "$1" != "--no-fetch" ]; then
	echo "Fetching new translations..."
	(
		# Clone the LP translation branch if it doesn't exist,
		# update otherwise
		if [ ! -d ./translations-export ]; then
			mkdir ./translations-export
			cd ./translations-export
			bzr branch lp:~stuntrally-team/stuntrally/pofiles
		else
			cd ./translations-export/pofiles
			bzr pull
		fi
	)
fi

echo "Generating languages..."
LOCALES="de fi ro pl fr ru it sk"
for loc in $LOCALES; do
	`which python2` ./xml_po_parser.py ./translations-export/pofiles/stuntrally/${loc}.po ../data/gui/core_language_${loc}_tag.xml
done
# Special case for Brazilian Portuguese
`which python2` ./xml_po_parser.py ./translations-export/pofiles/stuntrally/pt_BR.po ../data/gui/core_language_pt_tag.xml

if [ ! -d ./translation_templates ]; then
	echo "Cloning translation_templates"
	git clone git@github.com:stuntrally/translation_templates.git
fi

if [ "$1" != "--no-upload" ]; then
	echo "Uploading new template..."
	cd translation_templates
	git pull origin master
	cp ../stuntrally.pot ./stuntrally/
	git add stuntrally/stuntrally.pot
	git commit -m "Automatic translation template update"
	git push origin master
fi

echo "Done."
