

# Global valiables
$build_version_file="build_version.h"
$build_number_file="BUILD"


# Get current build GIT revision (Current commits number)
$revision_command=(git rev-list --count HEAD)
$revision_string='#define VERSION_REVISION            '+$revision_command

# Save to version file
$revision_string | Out-File $build_version_file


# Get current build number. If it doesn't exist create new with build number 1
$build_number=1
if (Test-Path $build_number_file)
{
	# Read old number from file
	$old_build_number_str=Get-Content $build_number_file
	$old_build_number=[System.Decimal]::Parse($old_build_number_str)

	# Increment number	
	$build_number=$old_build_number + 1;
	
	# Save number to file for future
	$build_number_str="{0:0000}" -f $build_number
	$build_number_str | Out-File $build_number_file
}
else
{
	# If it doesn't exist create new with build number 1
	$build_number_str="{0:0000}" -f $build_number
	$build_number_str | Out-File $build_number_file
}

	# Save to version file
	$build_number_full_string='#define VERSION_BUILD                '+$build_number_str
	Add-Content $build_version_file $build_number_full_string

exit 0
