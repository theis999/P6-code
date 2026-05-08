using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc.RazorPages;
using System.Security.Claims.ClaimTypes.NameIdentifier;
using System.Text.Json;

namespace newnewWebinterface.Pages;

[Authorize]
public class RegisterModel : PageModel
{
    public string name;
    public string ProductID;

    string authentikUserID = User.FindFirst(ClaimTypes.NameIdentifier)?.Value;



    public async Task<IActionResult> OnPostAsync()
    {
        
        var content = new JsonObject
        {
            ["name"] = name,
            ["ProductID"] = ProductID,
            ["authentikUserID"] = authentikUserID
        };

        await client.PostAsync("smakdb.head9x.dk/boards", content);

    }
}
